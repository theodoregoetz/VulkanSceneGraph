/* <editor-fold desc="MIT License">

Copyright(c) 2018 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/io/Options.h>
#include <vsg/io/stream.h>
#include <vsg/nodes/DepthSorted.h>

using namespace vsg;

DepthSorted::DepthSorted(Allocator* allocator) :
    Inherit(allocator)
{
}

DepthSorted::DepthSorted(int32_t in_bin, const dvec3& in_center, ref_ptr<Node> in_child, Allocator* allocator) :
    Inherit(allocator),
    bin(in_bin),
    center(in_center),
    child(in_child)
{
}

DepthSorted::~DepthSorted()
{
}

void DepthSorted::read(Input& input)
{
    Node::read(input);

    input.read("bin", bin);
    input.read("center", center);
    input.readObject("child", child);
}

void DepthSorted::write(Output& output) const
{
    Node::write(output);

    output.write("bin", bin);
    output.write("center", center);
    output.writeObject("child", child.get());
}