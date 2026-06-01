#ifndef RUNCPP2_DATA_PARAMETER_VALUE_HPP
#define RUNCPP2_DATA_PARAMETER_VALUE_HPP

#include "runcpp2/LibYAML_Wrapper.hpp"
#include "runcpp2/ParseUtil.hpp"

#include "mpark/variant.hpp"
#include "DSResult/DSResult.hpp"

#include <string>
#include <vector>
#include <utility>
#include <unordered_map>
#include <exception>

namespace runcpp2
{
namespace Data
{
    struct ParameterValue
    {
        enum class ConstraintType
        {
            None,
            Bool,
            Float,
            Int,
            Choices,
            Count       //5
        };
        
        bool Optional = true;
        std::string Default = "";
        bool Array = false;
        ConstraintType CurrentConstraintType = ConstraintType::None;
        
        mpark::variant<std::string, std::vector<std::string>> ConstraintValue = "";
        
        //Parsed substituted value
        mpark::variant<std::string, std::vector<std::string>> SubstitutedValue = "";
    
        inline DS::Result<std::pair<float, float>> GetConstraintFloats() const
        {
            DS_ASSERT_EQ((int)CurrentConstraintType, (int)ConstraintType::Float);
            DS_ASSERT_TRUE(mpark::is<std::string>(ConstraintValue));
            
            std::string constraintValue = mpark::get<std::string>(ConstraintValue);
            DS_ASSERT_GT(constraintValue.size(), 6);
            constraintValue = constraintValue.substr(6);
            size_t foundIndex = constraintValue.find(",");
            if( foundIndex == std::string::npos || 
                constraintValue.find(",", foundIndex + 1) != std::string::npos)
            {
                return DS_ERROR_MSG("Invalid constraint value for float: " + constraintValue);
            }
            
            try
            {
                float minF = std::stof(constraintValue.substr(0, foundIndex));
                float maxF = std::stof(constraintValue.substr(foundIndex + 1));
                DS_ASSERT_LT(minF, maxF);
                return std::make_pair(minF, maxF);
            }
            catch(const std::exception& ex)
            {
                return DS_ERROR_MSG("Invalid constraint value for float: " + constraintValue);
            }
        }
        
        inline DS::Result<std::pair<int, int>> GetConstraintInts() const
        {
            DS_ASSERT_EQ((int)CurrentConstraintType, (int)ConstraintType::Int);
            DS_ASSERT_TRUE(mpark::is<std::string>(ConstraintValue));
            
            std::string constraintValue = mpark::get<std::string>(ConstraintValue);
            DS_ASSERT_GT(constraintValue.size(), 4);
            constraintValue = constraintValue.substr(4);
            size_t foundIndex = constraintValue.find(",");
            if( foundIndex == std::string::npos || 
                constraintValue.find(",", foundIndex + 1) != std::string::npos)
            {
                return DS_ERROR_MSG("Invalid constraint value for int: " + constraintValue);
            }
            
            try
            {
                int minI = std::stoi(constraintValue.substr(0, foundIndex));
                int maxI = std::stoi(constraintValue.substr(foundIndex + 1));
                DS_ASSERT_LT_EQ(minI, maxI);
                return std::make_pair(minI, maxI);
            }
            catch(const std::exception& ex)
            {
                return DS_ERROR_MSG("Invalid constraint value for int: " + constraintValue);
            }
        }
        
        inline DS::Result<std::unordered_map<std::string, std::string>> GetConstraintOptions() const
        {
            DS_ASSERT_EQ((int)CurrentConstraintType, (int)ConstraintType::Choices);
            DS_ASSERT_TRUE(mpark::is<std::vector<std::string>>(ConstraintValue));
            
            std::unordered_map<std::string, std::string> retMap;
            
            const std::vector<std::string>& choices = 
                mpark::get<std::vector<std::string>>(ConstraintValue);
            
            for(int i = 0; i < choices.size(); ++i)
            {
                size_t foundIndex = choices.at(i).find(":");
                if( foundIndex != std::string::npos && 
                    choices.at(i).find(":", foundIndex + 1) != std::string::npos)
                {
                    return DS_ERROR_MSG("Invalid mapping format for choice: " + choices.at(i));
                }
                
                if(foundIndex == std::string::npos)
                    retMap[choices.at(i)] = choices.at(i);
                else
                    retMap[choices.at(i).substr(0, foundIndex)] = choices.at(i).substr(foundIndex + 1);
            }
            
            return retMap;
        }
        
        inline DS::Result<bool> IsInputInConstraint(const std::string input) const
        {
            static_assert((int)ConstraintType::Count == 5, "");
            switch(CurrentConstraintType)
            {
                case ConstraintType::None:
                    return true;
                case ConstraintType::Bool:
                    return input == "true" || input == "false" || input == "0" || input == "1";
                case ConstraintType::Float:
                {
                    std::pair<float, float> minMax = GetConstraintFloats().DS_TRY();
                    float parsedFloat;
                    try
                    {
                        parsedFloat = std::stof(input);
                    }
                    catch(const std::exception& ex)
                    {
                        return DS_ERROR_MSG("Failed to parse input as float: " + input);
                    }
                    return parsedFloat >= minMax.first && parsedFloat <= minMax.second;
                }
                case ConstraintType::Int:
                {
                    std::pair<int, int> minMax = GetConstraintInts().DS_TRY();
                    int parsedInt;
                    try
                    {
                        parsedInt = std::stoi(input);
                    }
                    catch(const std::exception& ex)
                    {
                        return DS_ERROR_MSG("Failed to parse input as int: " + input);
                    }
                    return parsedInt >= minMax.first && parsedInt <= minMax.second;
                }
                case ConstraintType::Choices:
                {
                    std::unordered_map< std::string, 
                                        std::string> options = GetConstraintOptions().DS_TRY();
                    return options.count(input) != 0;
                }
            }
            return false;
        }
        
        inline DS::Result<void> ParseYAML_Node(YAML::ConstNodePtr node)
        {
            std::vector<NodeRequirement> requirements =
            {
                NodeRequirement("Optional", YAML::NodeType::Scalar, false, false),
                NodeRequirement("Default", YAML::NodeType::Scalar, false, true),
                NodeRequirement("Array", YAML::NodeType::Scalar, false, false)
            };
            
            if(!CheckNodeRequirements(node, requirements))
                return DS_ERROR_MSG("ParameterValue: Failed to meet requirements");
            
            if(ExistAndHasChild(node, "Optional"))
            {
                Optional = node->GetMapValueScalar<bool>("Optional").DS_TRY();
            }
            
            if(ExistAndHasChild(node, "Array"))
            {
                Array = node->GetMapValueScalar<bool>("Array").DS_TRY();
            }
            
            if(ExistAndHasChild(node, "Constraint"))
            {
                if(node->GetMapValueNode("Constraint")->IsScalar())
                {
                    static_assert((int)ConstraintType::Count == 5, "");
                    std::string constraint = node->GetMapValueScalar<std::string>("Constraint").DS_TRY();
                    if(constraint == "None")
                        CurrentConstraintType = ConstraintType::None;
                    else if(constraint == "Bool")
                        CurrentConstraintType = ConstraintType::Bool;
                    else if(constraint.size() > 6 && constraint.substr(0, 6) == "Float:")
                    {
                        CurrentConstraintType = ConstraintType::Float;
                        ConstraintValue = constraint;
                        std::pair<float, float> parsed = GetConstraintFloats().DS_TRY();
                        (void)parsed;
                    }
                    else if(constraint.size() > 4 && constraint.substr(0, 4) == "Int:")
                    {
                        CurrentConstraintType = ConstraintType::Int;
                        ConstraintValue = constraint;
                        std::pair<int, int> parsed = GetConstraintInts().DS_TRY();
                        (void)parsed;
                    }
                }
                else if(node->GetMapValueNode("Constraint")->IsSequence())
                {
                    runcpp2::YAML::ConstNodePtr constraintSeq = node->GetMapValueNode("Constraint");
                    std::vector<std::string> constraints;
                    for(int i = 0; i < constraintSeq->GetChildrenCount(); ++i)
                    {
                        if(!constraintSeq->GetSequenceChildNode(i)->IsScalar())
                        {
                            return DS_ERROR_MSG("ParameterValue: Constraint sequence can only contain"
                                                "scalar values");
                        }
                        std::string constraintVal = 
                            constraintSeq->GetSequenceChildScalar<std::string>(i).DS_TRY();
                        constraints.emplace_back(std::move(constraintVal));
                    }
                    CurrentConstraintType = ConstraintType::Choices;
                    ConstraintValue = std::move(constraints);
                    
                    std::unordered_map< std::string, 
                                        std::string> parsed = GetConstraintOptions().DS_TRY();
                    (void)parsed;
                }
                else
                    return DS_ERROR_MSG("ParameterValue: Unexpected node type for constraint");
            }
            
            if(ExistAndHasChild(node, "Default", true))
            {
                Default = node->GetMapValueScalar<std::string>("Default").DS_TRY();
            }
            
            return {};
        }
        
        inline DS::Result<std::string> ToString(const std::string& indentation) const
        {
            std::string out;
            
            out += indentation + "Optional: " + (Optional ? "true" : "false") + "\n";
            out += indentation + "Default: \"" + Default + "\"\n";
            out += indentation + "Array: " + (Array ? "true" : "false") + "\n";
            
            static_assert((int)ConstraintType::Count == 5, "");
            switch(CurrentConstraintType)
            {
                case ConstraintType::None:
                    out += indentation + "Constraint: \"None\"\n";
                    break;
                case ConstraintType::Bool:
                    out += indentation + "Constraint: \"Bool\"\n";
                    break;
                case ConstraintType::Float:
                case ConstraintType::Int:
                    DS_ASSERT_TRUE(mpark::is<std::string>(ConstraintValue));
                    out +=  indentation + 
                            "Constraint: \"" + 
                            mpark::get<std::string>(ConstraintValue) + 
                            "\"\n";
                    break;
                case ConstraintType::Choices:
                {
                    DS_ASSERT_TRUE(mpark::is<std::vector<std::string>>(ConstraintValue));
                    out += indentation + "Constraint: [";
                    const std::vector<std::string>& constraintVals = 
                        mpark::get<std::vector<std::string>>(ConstraintValue);
                    
                    if(!constraintVals.empty())
                    {
                        for(int i = 0; i < constraintVals.size() - 1; ++i)
                            out += "\"" + constraintVals[i] + "\", ";
                        out += "\"" + constraintVals.back() + "\"";
                    }
                    out += "]\n";
                }
            }
            
            return out;
        }
        
        inline bool Equals(const ParameterValue& other) const
        {
            return  Optional == other.Optional && 
                    Default == other.Default && 
                    Array == other.Array &&
                    CurrentConstraintType == other.CurrentConstraintType &&
                    ConstraintValue == other.ConstraintValue;
        }
    };




}
}


#endif
