/* <editor-fold desc="MIT License">

Copyright(c) 2018 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/core/Version.h>

#include <vsg/io/BinaryOutput.h>

using namespace vsg;

BinaryOutput::BinaryOutput(std::ostream& output, ref_ptr<const Options> in_options) :
    Output(in_options),
    _output(output)
{
}

void BinaryOutput::_write(const std::string& str)
{
    uint32_t size = static_cast<uint32_t>(str.size());
    _output.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
    _output.write(str.data(), size);
}

void BinaryOutput::_write(const std::wstring& str)
{
    std::string string_value;
    convert_utf(str, string_value);
    _write(string_value);
}

void BinaryOutput::write(size_t num, const std::string* value)
{
    if (num == 1)
    {
        _write(*value);
    }
    else
    {
        for (; num > 0; --num, ++value)
        {
            _write(*value);
        }
    }
}

void BinaryOutput::write(size_t num, const std::wstring* value)
{
    if (num == 1)
    {
        _write(*value);
    }
    else
    {
        for (; num > 0; --num, ++value)
        {
            _write(*value);
        }
    }
}

void BinaryOutput::write(size_t num, const Path* value)
{
    if (num == 1)
    {
        _write(value->string());
    }
    else
    {
        for (; num > 0; --num, ++value)
        {
            _write(value->string());
        }
    }
}

void BinaryOutput::write(size_t num, const long double* value)
{
    uint32_t write_type = native_long_double_bits();
    _write(1, &write_type);
    _write(num, value);
}

void BinaryOutput::write(const vsg::Object* object)
{
    if (auto itr = objectIDMap.find(object); itr != objectIDMap.end())
    {
        // write out the objectID
        uint32_t id = itr->second;
        _output.write(reinterpret_cast<const char*>(&id), sizeof(id));
        return;
    }

    ObjectID id = objectID++;
    objectIDMap[object] = id;

    _output.write(reinterpret_cast<const char*>(&id), sizeof(id));
    if (object)
    {
        _write(std::string(object->className()));
        object->write(*this);
    }
    else
    {
        _write(std::string("nullptr"));
    }
}
