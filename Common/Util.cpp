#include <comdef.h>

#include "Util.h"

DxException::DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number)
:error_code(hr), function_name(function_name), filename(filename), line_number(line_number)
{
}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(error_code);
    std::wstring msg = err.ErrorMessage();

    return function_name + L" failed in " + filename + L"; line " + std::to_wstring(line_number) + L"; error: " + msg;
}