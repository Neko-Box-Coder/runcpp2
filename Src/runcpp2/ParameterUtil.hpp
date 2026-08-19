#ifndef RUNCPP2_PARAMETER_UTIL_HPP
#define RUNCPP2_PARAMETER_UTIL_HPP

#include "runcpp2/Data/ParameterValue.hpp"
#include "runcpp2/ParseUtil.hpp"

namespace runcpp2
{
    //Expecting `std::unordered_map<std::string, ParameterValue> Parameters;` and
    //`std::unordered_map<std::string, std::string> Variables;` from T
    template<typename T>
    inline DS::Result<void> ParseParametersAndVariables(T& data, YAML::ConstNodePtr node)
    {
        std::vector<NodeRequirement> requirements =
        {
            NodeRequirement("Parameters", YAML::NodeType::Map, false, true),
            NodeRequirement("Variables", YAML::NodeType::Map, false, true),
        };
        
        DS_ASSERT_TRUE(CheckNodeRequirements(node, requirements));
        
        //NOTE: Evaluate Parameters first, since we need to perform substitutions
        if(ExistAndHasChild(node, "Parameters"))
        {
           YAML::ConstNodePtr parametersNode = node->GetMapValueNode("Parameters");
           for(int i = 0; i < parametersNode->GetChildrenCount(); ++i)
           {
                std::string parameterName = parametersNode->GetMapKeyScalarAt<std::string>(i).DS_TRY();
                YAML::ConstNodePtr valueNode = parametersNode->GetMapValueNodeAt(i);
                Data::ParameterValue paramValue;
                paramValue.ParseYAML_Node(valueNode).DS_TRY();
                if(data.Parameters.count(parameterName) != 0)
                {
                    return DS_ERROR_MSG("ScriptInfo: Same parameter (" + parameterName + 
                                        ") is added more than once");
                }
                data.Parameters[parameterName] = paramValue;
           }
        }
        
        //NOTE: Evaluate Parameters first, since we need to perform substitutions
        if(ExistAndHasChild(node, "Variables"))
        {
            YAML::ConstNodePtr variablesNode = node->GetMapValueNode("Variables");
            for(int i = 0; i < variablesNode->GetChildrenCount(); ++i)
            {
                std::string variableName = variablesNode->GetMapKeyScalarAt<std::string>(i).DS_TRY();
                std::string variableValue = variablesNode->GetMapValueScalarAt<std::string>(i).DS_TRY();
                if(data.Variables.count(variableName) != 0)
                    return DS_ERROR_MSG("Same variable (" + variableName + ") is added more than once");
                data.Variables[variableName] = variableValue;
            }
        }
        
        return {};
    }


    //Expecting `std::unordered_map<std::string, ParameterValue> Parameters;` and
    //`std::unordered_map<std::string, std::string> Variables;` from T
    template<typename T>
    inline DS::Result<void> 
    ApplyParametersAndVariables(T& data, 
                                YAML::NodePtr& node,
                                YAML::ResourceHandle& resourceHandle,
                                std::unordered_map< std::string, 
                                                    std::vector<std::string>>& substitutionMap,
                                const std::unordered_map<std::string, std::string>& inputParameters,
                                const std::vector<YAML::ConstNodePtr>& excludedNodes)
    {
        //Remove parameters and variables in the cloned node.
        if(node->HasMapKey("Parameters"))
        {
            node->RemoveMapChild("Parameters").DS_TRY();
        }
        if(node->HasMapKey("Variables"))
        {
            node->RemoveMapChild("Variables").DS_TRY();
        }
        
        //Populate parameters values first
        for(auto it = data.Parameters.begin(); it != data.Parameters.end(); ++it)
        {
            bool isDefault = true;
            std::string valueToParse;
            if(inputParameters.count(it->first) == 0)
                valueToParse = it->second.Default;
            else
            {
                valueToParse = inputParameters.at(it->first);
                isDefault = false;
            }
            
            //Parse the value
            std::string subKey = "{" + it->first + "}";
            
            using ConstraintType = Data::ParameterValue::ConstraintType;
            
            //Check if it is optional, if so check if it can be empty
            if(it->second.Optional && valueToParse.empty())
            {
                static_assert((int)ConstraintType::Count == 5, "");
                if(it->second.CurrentConstraintType != ConstraintType::None)
                    continue;
            }
            
            if(it->second.Array)
            {
                std::vector<std::string> inputValues;
                SplitString(valueToParse, ",", inputValues);
                std::unordered_map< std::string, 
                                    std::string> options = it->second.GetConstraintOptions().DS_TRY();
                for(int i = 0; i < inputValues.size(); ++i)
                {
                    bool inConstraint = it->second.IsInputInConstraint(inputValues[i]).DS_TRY();
                    if(!inConstraint)
                    {
                        return DS_ERROR_MSG("Input parameter[" + DS_STR(i) + "]: " + 
                                            inputValues[i] +
                                            (isDefault ? "(Default)" : "") + 
                                            " is not in constraint");
                    }
                    if(it->second.CurrentConstraintType == ConstraintType::Choices)
                        inputValues[i] = options[inputValues[i]];
                }
                substitutionMap[subKey] = inputValues;
            }
            else
            {
                bool inConstraint = it->second.IsInputInConstraint(valueToParse).DS_TRY();
                if(!inConstraint)
                {
                    return DS_ERROR_MSG("Input parameter: " + it->first +
                                        (isDefault ? "(Default)" : "") + 
                                        " is not in constraint");
                }
                if(it->second.CurrentConstraintType != ConstraintType::Choices)
                    substitutionMap[subKey] = {valueToParse};
                else
                {
                    std::unordered_map< std::string, 
                                        std::string> options = it   ->second
                                                                    .GetConstraintOptions()
                                                                    .DS_TRY();
                    substitutionMap[subKey] = {options[valueToParse]};
                }
            }
        } //for(auto it = Parameters.begin(); it != Parameters.end(); ++it)
        
        std::unordered_map<std::string, std::vector<std::string>> variablesMap;
        
        //Populate variables next
        for(auto it = data.Variables.begin(); it != data.Variables.end(); ++it)
        {
            //Check if the same key already exists in the parameter
            if(data.Parameters.count(it->first) != 0)
            {
                return DS_ERROR_MSG("Variable name " + 
                                    it->first + 
                                    " already exists in Parameters");
            }
            
            std::string subKey = "{" + it->first + "}";
            variablesMap[subKey] = {};
            PerformMultiSubstitutions(  substitutionMap, 
                                        {}, 
                                        it->second, 
                                        variablesMap[subKey]).DS_TRY();
        }
        substitutionMap.insert(variablesMap.begin(), variablesMap.end());
        
        //NOTE: We still need to iterate through the whole thing even if we have nothing to 
        //      substitute because we need to get the escaped strings
        
        //Perform substitution recursively over the whole YAML object
        std::deque<YAML::NodePtr> nodesToVisit;
        nodesToVisit.push_back(node);
        while(!nodesToVisit.empty())
        {
            YAML::NodePtr currentNode = nodesToVisit.front();
            nodesToVisit.pop_front();
            
            bool skipThis = false;
            for(int i = 0; i < excludedNodes.size(); ++i)
            {
                if(excludedNodes.at(i).get() == currentNode.get())
                {
                    skipThis = true;
                    break;
                }
            }
            if(skipThis)
                continue;
            
            static_assert((int)YAML::NodeType::Count == 4, "");
            switch(currentNode->GetType())
            {
                case YAML::NodeType::Scalar:
                {
                    YAML::Node* parent = currentNode->GetParent();
                    std::string scalarValue = currentNode->GetScalar<std::string>().DS_TRY();
                    if(parent && parent->GetType() == YAML::NodeType::Sequence)
                    {
                        std::vector<std::string> newValues;
                        PerformMultiSubstitutions(  substitutionMap, 
                                                    {}, 
                                                    scalarValue, 
                                                    newValues).DS_TRY();
                        if(newValues.empty())
                            return DS_ERROR_MSG("Substitution array returned empty");
                        
                        //Update the current value
                        currentNode->InitScalar(newValues[0], resourceHandle).DS_TRY();
                        
                        //Find the index of the current node
                        int currentIndex = -1;
                        for(int i = 0; i < parent->GetChildrenCount(); ++i)
                        {
                            if(parent->GetSequenceChildNode(i) == currentNode)
                            {
                                currentIndex = i;
                                break;
                            }
                        }
                        
                        if(currentIndex == -1)
                            return DS_ERROR_MSG("Cannot find current node from parent?");
                        
                        //Then insert the rest of the substituted values after the current one
                        if(newValues.size() > 1)
                        {
                            for(int i = 1; i < newValues.size(); ++i)
                            {
                                YAML::NodePtr newChild = 
                                    parent->CreateSequenceChildAt(currentIndex + i).DS_TRY();
                                newChild->InitScalar(newValues[i], resourceHandle).DS_TRY();
                            }
                        }
                    }
                    else
                    {
                        PerformSubstitutions(substitutionMap, {}, scalarValue).DS_TRY();
                        currentNode->InitScalar(scalarValue, resourceHandle).DS_TRY();
                    }
                    break;
                } //case YAML::NodeType::Scalar:
                case YAML::NodeType::Alias:
                    return DS_ERROR_MSG("Anchors should be resolved. This should not be reached");
                case YAML::NodeType::Sequence:
                    for(int i = currentNode->GetChildrenCount() - 1; i >= 0; --i)
                        nodesToVisit.push_back(currentNode->GetSequenceChildNode(i));
                    break;
                case YAML::NodeType::Map:
                    for(int i = currentNode->GetChildrenCount() - 1; i >= 0; --i)
                        nodesToVisit.push_back(currentNode->GetMapValueNodeAt(i));
                    break;
            } //switch(currentNode->GetType())
        } //while(!nodesToVisit.empty())
    
        return {};
    }
    
    inline void CreateParameterValues(  const std::string rawParams,
                                        std::unordered_map<std::string, std::string>& outParameters)
    {
        std::vector<std::string> paramNameVals;
        SplitString(rawParams, ":", paramNameVals);
        if(paramNameVals.size() % 2 != 0)
        {
            ssLOG_ERROR("Failed to parse parameters. Defaults to no parameters");
            return;
        }
        
        for(int i = 0; i < paramNameVals.size(); i += 2)
            outParameters[paramNameVals[i]] = paramNameVals[i + 1];
    }
}


#endif
