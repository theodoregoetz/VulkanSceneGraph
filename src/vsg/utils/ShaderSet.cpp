/* <editor-fold desc="MIT License">

Copyright(c) 2022 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include <vsg/io/Input.h>
#include <vsg/io/Logger.h>
#include <vsg/io/Options.h>
#include <vsg/io/Output.h>
#include <vsg/io/read.h>
#include <vsg/state/ColorBlendState.h>
#include <vsg/state/DescriptorImage.h>
#include <vsg/state/InputAssemblyState.h>
#include <vsg/state/MultisampleState.h>
#include <vsg/state/PipelineLayout.h>
#include <vsg/state/RasterizationState.h>
#include <vsg/state/TessellationState.h>
#include <vsg/state/VertexInputState.h>
#include <vsg/state/ViewDependentState.h>
#include <vsg/state/material.h>
#include <vsg/utils/ShaderSet.h>

#include "shaders/flat_ShaderSet.cpp"
#include "shaders/pbr_ShaderSet.cpp"
#include "shaders/phong_ShaderSet.cpp"

using namespace vsg;

int AttributeBinding::compare(const AttributeBinding& rhs) const
{
    if (name < rhs.name) return -1;
    if (name > rhs.name) return 1;

    if (define < rhs.define) return -1;
    if (define > rhs.define) return 1;

    int result = compare_value(location, rhs.location);
    if (result) return result;

    if ((result = compare_value(format, rhs.format))) return result;
    return compare_pointer(data, rhs.data);
}

int BufferBinding::compare(const BufferBinding& rhs) const
{
    if (name < rhs.name) return -1;
    if (name > rhs.name) return 1;

    if (define < rhs.define) return -1;
    if (define > rhs.define) return 1;

    int result = compare_value(set, rhs.set);
    if (result) return result;

    if ((result = compare_value(binding, rhs.binding))) return result;
    if ((result = compare_value(descriptorType, rhs.descriptorType))) return result;
    if ((result = compare_value(descriptorCount, rhs.descriptorCount))) return result;
    if ((result = compare_value(stageFlags, rhs.stageFlags))) return result;
    return compare_pointer(data, rhs.data);
}

int PushConstantRange::compare(const PushConstantRange& rhs) const
{
    if (name < rhs.name) return -1;
    if (name > rhs.name) return 1;

    if (define < rhs.define) return -1;
    if (define > rhs.define) return 1;

    return compare_region(range, range, rhs.range);
}

int DefinesArrayState::compare(const DefinesArrayState& rhs) const
{
    int result = compare_container(defines, rhs.defines);
    if (result) return result;

    return compare_pointer(arrayState, rhs.arrayState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// CustomDescriptorSetBinding
//
CustomDescriptorSetBinding::CustomDescriptorSetBinding(uint32_t in_set) :
    set(in_set)
{
}

int CustomDescriptorSetBinding::compare(const Object& rhs_object) const
{
    int result = Object::compare(rhs_object);
    if (result != 0) return result;

    auto& rhs = static_cast<decltype(*this)>(rhs_object);
    return compare_value(set, rhs.set);
}

void CustomDescriptorSetBinding::read(Input& input)
{
    Object::read(input);

    input.read("set", set);
}

void CustomDescriptorSetBinding::write(Output& output) const
{
    Object::write(output);

    output.write("set", set);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ViewDependentStateBinding
//
ViewDependentStateBinding::ViewDependentStateBinding(uint32_t in_set) :
    Inherit(in_set),
    viewDescriptorSetLayout(ViewDescriptorSetLayout::create())
{
}

int ViewDependentStateBinding::compare(const Object& rhs) const
{
    return CustomDescriptorSetBinding::compare(rhs);
}

void ViewDependentStateBinding::read(Input& input)
{
    CustomDescriptorSetBinding::read(input);
}

void ViewDependentStateBinding::write(Output& output) const
{
    CustomDescriptorSetBinding::write(output);
}

ref_ptr<DescriptorSetLayout> ViewDependentStateBinding::createDescriptorSetLayout()
{
    return viewDescriptorSetLayout;
}

ref_ptr<StateCommand> ViewDependentStateBinding::createStateCommand(ref_ptr<PipelineLayout> layout)
{
    return BindViewDescriptorSets::create(VK_PIPELINE_BIND_POINT_GRAPHICS, layout, set);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//
// ShaderSet
//
ShaderSet::ShaderSet()
{
}

ShaderSet::ShaderSet(const ShaderStages& in_stages, ref_ptr<ShaderCompileSettings> in_hints) :
    stages(in_stages),
    defaultShaderHints(in_hints)
{
}

ShaderSet::~ShaderSet()
{
}

void ShaderSet::addAttributeBinding(std::string name, std::string define, uint32_t location, VkFormat format, ref_ptr<Data> data)
{
    attributeBindings.push_back(AttributeBinding{name, define, location, format, data});
}

void ShaderSet::addBufferBinding(std::string name, std::string define, uint32_t set, uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags, ref_ptr<Data> data)
{
    bufferBindings.push_back(BufferBinding{name, define, set, binding, descriptorType, descriptorCount, stageFlags, data});
}

/// deprecated. use ShaderSet::addBufferBinding() instead
void ShaderSet::addUniformBinding(std::string name, std::string define, uint32_t set, uint32_t binding, VkDescriptorType descriptorType, uint32_t descriptorCount, VkShaderStageFlags stageFlags, ref_ptr<Data> data)
{
    warn("ShaderSet::addUniformBinding() has been deprecated."
         " use ShaderSet::addBufferBinding() instead.");
    return addBufferBinding(name, define, set, binding, descriptorType, descriptorCount, stageFlags, data);
}

void ShaderSet::addPushConstantRange(std::string name, std::string define, VkShaderStageFlags stageFlags, uint32_t offset, uint32_t size)
{
    pushConstantRanges.push_back(vsg::PushConstantRange{name, define, VkPushConstantRange{stageFlags, offset, size}});
}

const AttributeBinding& ShaderSet::getAttributeBinding(const std::string& name) const
{
    for (auto& binding : attributeBindings)
    {
        if (binding.name == name) return binding;
    }
    return _nullAttributeBinding;
}

BufferBinding& ShaderSet::getBufferBinding(const std::string& name)
{
    for (auto& binding : bufferBindings)
    {
        if (binding.name == name) return binding;
    }
    return _nullBufferBinding;
}

/// deprecated. use ShaderSet::getBufferBinding() instead
BufferBinding& ShaderSet::getUniformBinding(const std::string& name)
{
    warn("ShaderSet::getUniformBinding() has been deprecated."
         " use ShaderSet::getBufferBinding() instead.");
    return getBufferBinding(name);
}

AttributeBinding& ShaderSet::getAttributeBinding(const std::string& name)
{
    for (auto& binding : attributeBindings)
    {
        if (binding.name == name) return binding;
    }
    return _nullAttributeBinding;
}

const BufferBinding& ShaderSet::getBufferBinding(const std::string& name) const
{
    for (auto& binding : bufferBindings)
    {
        if (binding.name == name) return binding;
    }
    return _nullBufferBinding;
}

/// deprecated. use ShaderSet::getBufferBinding() instead.
const BufferBinding& ShaderSet::getUniformBinding(const std::string& name) const
{
    warn("ShaderSet::getUniformBinding() has been deprecated."
         " use ShaderSet::getBufferBinding() instead.");
    return getBufferBinding(name);
}

ref_ptr<ArrayState> ShaderSet::getSuitableArrayState(const std::set<std::string>& defines) const
{
    // not all defines are relevant to the provided ArrayState
    // so check each one against the entries in the definesArrayState
    // relevant to the final matching.
    std::set<std::string> relevant_defines;
    for (auto& define : defines)
    {
        for (auto& definesArrayState : definesArrayStates)
        {
            if (definesArrayState.defines.count(define) != 0)
            {
                relevant_defines.insert(define);
                break;
            }
        }
    }

    // find the matching ArrayState
    for (auto& definesArrayState : definesArrayStates)
    {
        if (definesArrayState.defines == relevant_defines)
        {
            return definesArrayState.arrayState;
        }
    }

    return {};
}

ShaderStages ShaderSet::getShaderStages(ref_ptr<ShaderCompileSettings> scs)
{
    std::scoped_lock<std::mutex> lock(mutex);

    if (auto itr = variants.find(scs); itr != variants.end())
    {
        return itr->second;
    }

    auto& new_stages = variants[scs];
    for (auto& stage : stages)
    {
        if (vsg::compare_pointer(stage->module->hints, scs) == 0)
        {
            new_stages.push_back(stage);
        }
        else
        {
            auto new_stage = vsg::ShaderStage::create();
            new_stage->flags = stage->flags;
            new_stage->stage = stage->stage;
            new_stage->module = ShaderModule::create(stage->module->source, scs);
            new_stage->entryPointName = stage->entryPointName;
            new_stage->specializationConstants = stage->specializationConstants;
            new_stages.push_back(new_stage);
        }
    }

    return new_stages;
}

int ShaderSet::compare(const Object& rhs_object) const
{
    int result = Object::compare(rhs_object);
    if (result != 0) return result;

    auto& rhs = static_cast<decltype(*this)>(rhs_object);
    if ((result = compare_pointer_container(stages, rhs.stages))) return result;
    if ((result = compare_container(attributeBindings, rhs.attributeBindings))) return result;
    if ((result = compare_container(bufferBindings, rhs.bufferBindings))) return result;
    if ((result = compare_container(pushConstantRanges, rhs.pushConstantRanges))) return result;
    if ((result = compare_container(definesArrayStates, rhs.definesArrayStates))) return result;
    if ((result = compare_container(optionalDefines, rhs.optionalDefines))) return result;
    return compare_pointer_container(defaultGraphicsPipelineStates, rhs.defaultGraphicsPipelineStates);
}

void ShaderSet::read(Input& input)
{
    Object::read(input);

    input.readObjects("stages", stages);

    if (input.version_greater_equal(1, 0, 4))
    {
        input.readObject("defaultShaderHints", defaultShaderHints);
    }

    auto num_attributeBindings = input.readValue<uint32_t>("attributeBindings");
    attributeBindings.resize(num_attributeBindings);
    for (auto& binding : attributeBindings)
    {
        input.read("name", binding.name);
        input.read("define", binding.define);
        input.read("location", binding.location);
        input.readValue<uint32_t>("format", binding.format);
        input.readObject("data", binding.data);
    }

    auto num_bufferBindings = input.readValue<uint32_t>("bufferBindings");
    bufferBindings.resize(num_bufferBindings);
    for (auto& binding : bufferBindings)
    {
        input.read("name", binding.name);
        input.read("define", binding.define);
        input.read("set", binding.set);
        input.read("binding", binding.binding);
        input.readValue<uint32_t>("descriptorType", binding.descriptorType);
        input.read("descriptorCount", binding.descriptorCount);
        input.readValue<uint32_t>("stageFlags", binding.stageFlags);
        input.readObject("data", binding.data);
    }

    auto num_pushConstantRanges = input.readValue<uint32_t>("pushConstantRanges");
    pushConstantRanges.resize(num_pushConstantRanges);
    for (auto& pcr : pushConstantRanges)
    {
        input.read("name", pcr.name);
        input.read("define", pcr.define);
        input.readValue<uint32_t>("stageFlags", pcr.range.stageFlags);
        input.read("offset", pcr.range.offset);
        input.read("size", pcr.range.size);
    }

    auto num_definesArrayStates = input.readValue<uint32_t>("definesArrayStates");
    definesArrayStates.resize(num_definesArrayStates);
    for (auto& das : definesArrayStates)
    {
        input.readValues("defines", das.defines);
        input.readObject("arrayState", das.arrayState);
    }

    input.readValues("optionalDefines", optionalDefines);
    input.readObjects("defaultGraphicsPipelineStates", defaultGraphicsPipelineStates);

    auto num_variants = input.readValue<uint32_t>("variants");
    variants.clear();
    for (uint32_t i = 0; i < num_variants; ++i)
    {
        auto hints = input.readObject<ShaderCompileSettings>("hints");
        input.readObjects("stages", variants[hints]);
    }

    if (input.version_greater_equal(1, 0, 8))
    {
        auto num_custom = input.readValue<uint32_t>("customDescriptorSetBindings");
        customDescriptorSetBindings.clear();
        for (uint32_t i = 0; i < num_custom; ++i)
        {
            if (auto custom = input.readObject<CustomDescriptorSetBinding>("customDescriptorSetBinding"))
            {
                customDescriptorSetBindings.push_back(custom);
            }
        }
    }
}

void ShaderSet::write(Output& output) const
{
    Object::write(output);

    output.writeObjects("stages", stages);

    if (output.version_greater_equal(1, 0, 4))
    {
        output.writeObject("defaultShaderHints", defaultShaderHints);
    }

    output.writeValue<uint32_t>("attributeBindings", attributeBindings.size());
    for (auto& binding : attributeBindings)
    {
        output.write("name", binding.name);
        output.write("define", binding.define);
        output.write("location", binding.location);
        output.writeValue<uint32_t>("format", binding.format);
        output.writeObject("data", binding.data);
    }

    output.writeValue<uint32_t>("bufferBindings", bufferBindings.size());
    for (auto& binding : bufferBindings)
    {
        output.write("name", binding.name);
        output.write("define", binding.define);
        output.write("set", binding.set);
        output.write("binding", binding.binding);
        output.writeValue<uint32_t>("descriptorType", binding.descriptorType);
        output.write("descriptorCount", binding.descriptorCount);
        output.writeValue<uint32_t>("stageFlags", binding.stageFlags);
        output.writeObject("data", binding.data);
    }

    output.writeValue<uint32_t>("pushConstantRanges", pushConstantRanges.size());
    for (auto& pcr : pushConstantRanges)
    {
        output.write("name", pcr.name);
        output.write("define", pcr.define);
        output.writeValue<uint32_t>("stageFlags", pcr.range.stageFlags);
        output.write("offset", pcr.range.offset);
        output.write("size", pcr.range.size);
    }

    output.writeValue<uint32_t>("definesArrayStates", definesArrayStates.size());
    for (auto& das : definesArrayStates)
    {
        output.writeValues("defines", das.defines);
        output.writeObject("arrayState", das.arrayState);
    }

    output.writeValues("optionalDefines", optionalDefines);
    output.writeObjects("defaultGraphicsPipelineStates", defaultGraphicsPipelineStates);

    output.writeValue<uint32_t>("variants", variants.size());
    for (auto& [hints, variant_stages] : variants)
    {
        output.writeObject("hints", hints);
        output.writeObjects("stages", variant_stages);
    }

    if (output.version_greater_equal(1, 0, 8))
    {
        output.writeValue<uint32_t>("customDescriptorSetBindings", customDescriptorSetBindings.size());
        for (auto& custom : customDescriptorSetBindings)
        {
            output.writeObject("customDescriptorSetBinding", custom);
        }
    }
}

ref_ptr<ShaderSet> vsg::createFlatShadedShaderSet(ref_ptr<const Options> options)
{
    if (options)
    {
        // check if a ShaderSet has already been assigned to the options object, if so return it
        if (auto itr = options->shaderSets.find("flat"); itr != options->shaderSets.end()) return itr->second;
    }
    return flat_ShaderSet();
}

ref_ptr<ShaderSet> vsg::createPhongShaderSet(ref_ptr<const Options> options)
{
    if (options)
    {
        // check if a ShaderSet has already been assigned to the options object, if so return it
        if (auto itr = options->shaderSets.find("phong"); itr != options->shaderSets.end()) return itr->second;
    }

    return phong_ShaderSet();
}

ref_ptr<ShaderSet> vsg::createPhysicsBasedRenderingShaderSet(ref_ptr<const Options> options)
{
    if (options)
    {
        // check if a ShaderSet has already been assigned to the options object, if so return it
        if (auto itr = options->shaderSets.find("pbr"); itr != options->shaderSets.end()) return itr->second;
    }

    return pbr_ShaderSet();
}

std::pair<uint32_t, uint32_t> ShaderSet::descriptorSetRange() const
{
    if (bufferBindings.empty()) return {0, 0};

    uint32_t minimum = std::numeric_limits<uint32_t>::max();
    uint32_t maximum = std::numeric_limits<uint32_t>::min();

    for (auto& binding : bufferBindings)
    {
        if (binding.set < minimum) minimum = binding.set;
        if (binding.set > maximum) maximum = binding.set;
    }

    return {minimum, maximum + 1};
}

ref_ptr<DescriptorSetLayout> ShaderSet::createDescriptorSetLayout(const std::set<std::string>& defines, uint32_t set) const
{
    DescriptorSetLayoutBindings bindings;
    for (auto& binding : bufferBindings)
    {
        if (binding.set == set)
        {
            if (binding.define.empty() || defines.count(binding.define) > 0)
            {
                bindings.push_back(VkDescriptorSetLayoutBinding{binding.binding, binding.descriptorType, binding.descriptorCount, binding.stageFlags, nullptr});
            }
        }
    }

    return DescriptorSetLayout::create(bindings);
}

ref_ptr<PipelineLayout> ShaderSet::createPipelineLayout(const std::set<std::string>& defines, std::pair<uint32_t, uint32_t> range) const
{
    DescriptorSetLayouts descriptorSetLayouts;

    uint32_t set = 0;
    for (; set < range.first; ++set)
    {
        descriptorSetLayouts.push_back(DescriptorSetLayout::create());
    }

    for (; set < range.second; ++set)
    {
        descriptorSetLayouts.push_back(createDescriptorSetLayout(defines, set));
    }

    PushConstantRanges activePushConstantRanges;
    for (auto& pcb : pushConstantRanges)
    {
        if (pcb.define.empty() || defines.count(pcb.define) > 0) activePushConstantRanges.push_back(pcb.range);
    }

    return vsg::PipelineLayout::create(descriptorSetLayouts, activePushConstantRanges);
}
