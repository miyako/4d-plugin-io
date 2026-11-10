#include "4DPluginAPI.h"
#include "PrivateTypes.h"
#include "EntryPoints.h"
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <set>

static std::set<PA_Unichar *> strings;
static std::map<PA_Handle, size_t> handles;
static std::map<PA_Picture, unsigned> pictures;
static bool nullEmpty;
static unsigned errors;
static CUTF16String expectedText;
static bool failHandle;

static void check(bool valid, const char *message)
{
    if (!valid) {
        std::cerr << message << '\n';
        ++errors;
    }
}

static PA_Unistring makeString(const PA_Unichar *text)
{
    size_t units = 0;
    if (text) while (text[units]) ++units;
    PA_Unistring result = {};
    result.fLength = static_cast<PA_long32>(units);
    if (units || !nullEmpty) {
        result.fString = new PA_Unichar[units + 1];
        if (units) std::memcpy(result.fString, text, units * sizeof(PA_Unichar));
        result.fString[units] = 0;
        PA_Unichar *buffer = result.fString;
        strings.insert(buffer);
    }
    return result;
}

static void freeString(PA_Unistring &text)
{
    PA_Unichar *buffer = text.fString;
    if (buffer) {
        if (strings.erase(buffer))
            delete[] buffer;
        else
            check(false, "duplicate/borrowed string disposal");
    }
    text = {};
}

static PA_Picture makePicture()
{
    PA_Picture picture = new unsigned(0x1234);
    pictures[picture] = 1;
    return picture;
}

static void releasePicture(PA_Picture picture)
{
    if (!picture) return;
    auto it = pictures.find(picture);
    check(it != pictures.end(), "invalid picture release");
    if (it != pictures.end() && --it->second == 0) {
        delete static_cast<unsigned *>(picture);
        pictures.erase(it);
    }
}

static void clear(PA_Variable &value)
{
    switch (value.fType) {
        case eVK_Unistring: freeString(value.uValue.fString); break;
        case eVK_Picture: releasePicture(value.uValue.fPicture); break;
        case eVK_ArrayUnicode:
            if (value.uValue.fArray.fData) {
                auto elements = reinterpret_cast<PA_Unistring *>(*value.uValue.fArray.fData);
                for (PA_long32 i = 0; i <= value.uValue.fArray.fNbElements; ++i)
                    freeString(elements[i]);
                PA_DisposeHandle(value.uValue.fArray.fData);
            }
            break;
        case eVK_Undefined:
        case eVK_Longint: break;
        default: check(false, "unexpected variable cleanup kind"); break;
    }
    value.fType = eVK_Undefined;
}

static void engine(short selector, EngineBlock *eb)
{
    switch (selector) {
        case EX_CREATE_UNISTRING:
            eb->fUniString1 = makeString(reinterpret_cast<PA_Unichar *>(eb->fHandle));
            break;
        case EX_DISPOSE_UNISTRING: freeString(eb->fUniString1); break;
        case EX_SET_UNISTRING: {
            PA_Unistring copy = makeString(reinterpret_cast<PA_Unichar *>(eb->fHandle));
            freeString(eb->fUniString1);
            eb->fUniString1 = copy;
            break;
        }
        case EX_CREATE_PICTURE: eb->fPicture = makePicture(); eb->fError = 0; break;
        case EX_DISPOSE_PICTURE: releasePicture(eb->fPicture); break;
        case EX_DUPLICATE_PICTURE: {
            PA_Picture picture = eb->fPicture;
            if (eb->fPicture) {
                check(pictures.count(picture), "retain dead picture");
                ++pictures.at(picture);
            }
            break;
        }
        case EX_GET_PICTURE_DATA:
            // Deliberately poisoned failed output: production must check error first.
            eb->fUniString1.fString = reinterpret_cast<PA_Unichar *>(1);
            eb->fUniString1.fLength = 100;
            eb->fError = eER_IncompatibleTypes;
            break;
        case EX_CLEAR_VARIABLE: clear(*static_cast<PA_Variable *>(eb->fPtr1)); break;
        case EX_HANDLE_MANAGER: {
            PA_Handle handle = eb->fParam1 == 1 ? NULL : eb->fHandle;
            switch (eb->fParam1) {
                case 1: {
                    if (failHandle) {
                        failHandle = false;
                        eb->fHandle = NULL;
                        eb->fError = eER_IncompatibleTypes;
                        return;
                    }
                    PA_Handle h = new char *;
                    *h = new char[eb->fParam2]();
                    handles[h] = eb->fParam2;
                    eb->fHandle = h;
                    break;
                }
                case 2:
                    check(handles.erase(handle) == 1, "invalid handle disposal");
                    delete[] *eb->fHandle;
                    delete eb->fHandle;
                    break;
                case 3: {
                    size_t size = eb->fParam2;
                    char *data = new char[size]();
                    std::memcpy(data, *eb->fHandle, std::min(size, handles.at(handle)));
                    delete[] *eb->fHandle;
                    *eb->fHandle = data;
                    handles.at(handle) = size;
                    break;
                }
                case 4: eb->fParam2 = handles.at(handle); break;
                case 5: eb->fParam3 = reinterpret_cast<sLONG_PTR>(*eb->fHandle); break;
                case 6: break;
                default: std::abort();
            }
            eb->fError = 0;
            break;
        }
        case EX_CONVERT_CHARSET_TO_CHARSET: {
            check(eb->fParam2 == eVTC_UTF_8 && eb->fParam4 == eVTC_UTF_16,
                  "unexpected charset conversion");
            CFStringRef text = CFStringCreateWithBytes(kCFAllocatorDefault,
                static_cast<UInt8 *>(eb->fPtr1), eb->fParam1, kCFStringEncodingUTF8, false);
            assert(text);
            size_t units = CFStringGetLength(text);
            size_t bytes = units * sizeof(PA_Unichar);
            check(static_cast<size_t>(eb->fParam3) >= bytes + sizeof(PA_Unichar),
                  "UTF16 capacity missing terminator");
            CFStringGetCharacters(text, CFRangeMake(0, units), static_cast<UniChar *>(eb->fPtr2));
            static_cast<PA_Unichar *>(eb->fPtr2)[units] = 0;
            eb->fParam3 = bytes;
            eb->fError = 0;
            CFRelease(text);
            break;
        }
        case EX_EXECUTE_COMMAND_BY_ID: {
            auto args = static_cast<PA_Variable **>(eb->fPtr1);
            switch (eb->fParam1) {
                case 678:
                    check(CUTF16String(args[0]->uValue.fString.fString,
                        args[0]->uValue.fString.fLength) == expectedText, "path Unicode mismatch");
                    clear(*args[1]);
                    PA_SetPictureVariable(args[1], makePicture());
                    break;
                case 1002:
                    check(CUTF16String(args[1]->uValue.fString.fString,
                        args[1]->uValue.fString.fLength) == expectedText, "format Unicode mismatch");
                    break;
                case 988: break;
                case 457:
                    PA_SetLongintVariable(args[1], 17);
                    PA_SetLongintVariable(args[2], 23);
                    break;
                case 679:
                    clear(*args[1]);
                    PA_SetPictureVariable(args[1], makePicture());
                    break;
                default: std::abort();
            }
            eb->fError = 0;
            break;
        }
        default: std::cerr << "unexpected engine selector " << selector << '\n'; std::abort();
    }
}

static CUTF16String utf16(const char *text)
{
    CFStringRef s = CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8);
    assert(s);
    std::vector<PA_Unichar> buffer(CFStringGetLength(s) + 1);
    CFStringGetCharacters(s, CFRangeMake(0, buffer.size() - 1), buffer.data());
    CFRelease(s);
    return CUTF16String(buffer.data(), buffer.size() - 1);
}

static void pictureCase(const std::string &name)
{
    CPicture wrapper;
    if (name == "path-alias-and-output") {
        for (const char *text : {"ascii", "\xe6\x97\xa5", "\xf0\x9f\x98\x80", ""}) {
            expectedText = utf16(text);
            CUTF8String path(reinterpret_cast<const uint8_t *>(text));
            wrapper.fromFileAtPath(&path);
            check(pictures.count(wrapper.getPicture()), "output owner did not survive arg clear");
        }
    } else if (name == "setBytes-temporary") {
        const uint8_t bytes[] = {1, 2, 3};
        for (int i = 0; i < 3; ++i) wrapper.setBytes(bytes, sizeof(bytes));
    } else if (name == "copy-byte-units") {
        for (const char *text : {".png", "\xe6\x97\xa5", "\xf0\x9f\x98\x80", ""}) {
            expectedText = utf16(text);
            PA_Picture p = wrapper.createCopy(text);
            check(pictures.count(p), "owning conversion return lost");
            PA_DisposePicture(p);
        }
    } else if (name == "callback-empty-owners") {
        unsigned width = 0, height = 0;
        wrapper.getSize(&width, &height);
        check(width == 17 && height == 23, "callback scalar mismatch");
        PA_Picture gray = wrapper.createGrayScale();
        PA_DisposePicture(gray);
        PA_Picture thumb = wrapper.createThumbnail(17, 23);
        PA_DisposePicture(thumb);
        check(pictures.count(wrapper.getPicture()), "borrowed callback input disposed");
    } else {
        std::abort();
    }
}

static void arrayCase()
{
    ARRAY_TEXT text;
    text.setSize(5);
    const char *fixtures[] = {"", "ascii", "\xe6\x97\xa5", "\xf0\x9f\x98\x80", "ascii"};
    for (unsigned i = 0; i < 5; ++i) {
        CUTF16String u = utf16(fixtures[i]);
        text.setUTF16StringAtIndex(&u, i);
    }
    PA_Variable array = PA_CreateVariable(eVK_Undefined);
    BytePtr raw = reinterpret_cast<BytePtr>(&array);
    text.toParamAtIndex(&raw, 1);
    for (unsigned i = 0; i < 5; ++i) {
        PA_Unistring borrowed = PA_GetStringInArray(array, i);
        check(CUTF16String(borrowed.fString, borrowed.fLength) == utf16(fixtures[i]),
              "array content mismatch");
        check(borrowed.fString ? borrowed.fString[borrowed.fLength] == 0 : borrowed.fLength == 0,
              "array terminator missing");
    }
    check(PA_GetStringInArray(array, 1).fString != PA_GetStringInArray(array, 4).fString,
          "independent array values share owner");
    ARRAY_TEXT copy;
    copy.fromParamAtIndex(&raw, 1);
    text.toParamAtIndex(&raw, 1);
    PA_ResizeArray(&array, 1);
    PA_ClearVariable(&array);
    CUTF16String copied;
    copy.copyUTF16StringAtIndex(&copied, 3);
    check(copied == utf16(fixtures[3]), "borrowed array input not copied");

    PA_Unichar value[] = {'x', 0};
    PA_Unistring owner = PA_CreateUnistring(value);
    PA_Variable wrong = PA_CreateVariable(eVK_Longint);
    PA_SetStringInArray(wrong, 0, &owner);
    PA_Variable nullArray = {};
    nullArray.fType = eVK_ArrayUnicode;
    PA_SetStringInArray(nullArray, 0, &owner);
    array = PA_CreateVariable(eVK_ArrayUnicode);
    PA_SetStringInArray(array, -1, &owner);
    PA_SetStringInArray(array, 1, &owner);
    PA_SetStringInArray(array, 0, NULL);
    PA_Unichar *buffer = owner.fString;
    check(strings.count(buffer), "rejected setter consumed caller owner");
    PA_DisposeUnistring(&owner);
    PA_ClearVariable(&array);

    C_TEXT scalar;
    scalar.setUTF16String(copied.data(), copied.size());
    PA_Unistring output = PA_CreateUnistring(value);
    raw = reinterpret_cast<BytePtr>(&output);
    scalar.toParamAtIndex(&raw, 1);
    check(CUTF16String(output.fString, output.fLength) == copied, "scalar content setter mismatch");
    PA_DisposeUnistring(&output);

    PA_Unistring input = PA_CreateUnistring(value);
    raw = reinterpret_cast<BytePtr>(&input);
    scalar.fromParamAtIndex(&raw, 1);
    PA_DisposeUnistring(&input);
    check(scalar.getUTF16Length() == 1 && scalar.getUTF16StringPtr()[0] == 'x',
          "scalar borrowed input not copied");
    output = PA_CreateUnistring(value);
    PA_Unistring *destination = &output;
    scalar.setReturn(reinterpret_cast<sLONG_PTR *>(&destination));
    PluginBlock params = {};
    params.fResult = &destination;
    PA_ReturnString(&params, const_cast<PA_Unichar *>(copied.c_str()));
    check(CUTF16String(output.fString, output.fLength) == copied, "return content setter mismatch");
    PA_DisposeUnistring(&output);
}

static void failureCase()
{
    failHandle = true;
    PA_Variable result = PA_ExecuteCommandByID(678, NULL, 0);
    check(result.fType == 0, "command allocation failure raw sentinel changed");
    check(PA_GetVariableKind(result) == eVK_Unistring, "raw sentinel normalization changed");
    // Raw kind zero is not an owned empty string. Do not clear the sentinel.
    failHandle = true;
    result = PA_ExecuteMethodByID(1, NULL, 0);
    check(result.fType == eVK_Undefined, "method allocation failure kind changed");
    PA_ClearVariable(&result);

    PA_Picture transferred = NULL;
    PA_Picture parameter = makePicture();
    PA_Picture returned = NULL;
    {
        CPicture wrapper;
        transferred = wrapper.createGrayScale();
        BytePtr raw = reinterpret_cast<BytePtr>(&parameter);
        wrapper.toParamAtIndex(&raw, 1);
        wrapper.toParamAtIndex(&raw, 1);
        wrapper.setReturn(reinterpret_cast<sLONG_PTR *>(&returned));
    }
    check(pictures.count(transferred) == 1, "owning handoff did not survive producer");
    check(pictures.at(transferred) == 3 && transferred == parameter && transferred == returned,
          "independent parameter/return retain counts");
    PA_DisposePicture(transferred);
    PA_DisposePicture(parameter);
    PA_DisposePicture(returned);
}

static void dispatcherCase()
{
    PA_long32 invalidID = 0, result = 42, length = 1;
    PA_Unichar path[] = {'x', 0};
    PA_Unistring text = PA_CreateUnistring(path);
    PA_Handle blob = NULL;
    BytePtr args[3] = {reinterpret_cast<BytePtr>(&text),
                      reinterpret_cast<BytePtr>(&text), NULL};
    FourDPackex(1, args, NULL, &result);
    check(result == 0, "stubbed fopen result");
    args[0] = reinterpret_cast<BytePtr>(&invalidID);
    args[1] = reinterpret_cast<BytePtr>(&blob);
    args[2] = reinterpret_cast<BytePtr>(&length);
    for (PA_long32 selector : {2, 3, 4, 5, 6}) {
        result = 42;
        FourDPackex(selector, args, NULL, &result);
        check(result == (selector == 2 ? 0 : -1), "dispatcher invalid-ID result");
    }
    PA_DisposeUnistring(&text);
}

int main(int argc, char **argv)
{
    assert(argc == 3);
    gCall4D = engine;
    nullEmpty = std::string(argv[2]) == "null-empty";
    std::string name = argv[1];
    if (name == "arrays-and-scalars") arrayCase();
    else if (name == "dispatcher-no-io") dispatcherCase();
    else if (name == "failure-and-handoff") failureCase();
    else pictureCase(name);
    check(strings.empty(), "unbalanced string owners");
    check(handles.empty(), "unbalanced handle owners");
    check(pictures.empty(), "unbalanced picture retains");
    if (errors) return 1;
    std::cout << name << ' ' << argv[2] << ": PASS\n";
}
