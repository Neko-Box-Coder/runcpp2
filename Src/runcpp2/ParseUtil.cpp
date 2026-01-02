#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/StringUtil.hpp"

#include <unordered_set>

runcpp2::NodeRequirement::NodeRequirement() :   Name(""),
                                                NodeType_LibYaml(YAML::NodeType::Scalar),
                                                Required(false),
                                                Nullable(true)
{
}
    
runcpp2::NodeRequirement::NodeRequirement(  const std::string& name, 
                                            YAML::NodeType nodeType, 
                                            bool required,
                                            bool nullable) :    Name(name), 
                                                                NodeType_LibYaml(nodeType),
                                                                Required(required), 
                                                                Nullable(nullable)
{}

bool runcpp2::CheckNodeRequirement_LibYaml( YAML::ConstNodePtr parentNode, 
                                            const std::string childName, 
                                            YAML::NodeType childType,
                                            bool required,
                                            bool nullable)
{
    ssLOG_FUNC_DEBUG();
    
    if(!parentNode->IsMap())
    {
        ssLOG_ERROR("Node is not a map: " << parentNode->Value.index());
        return false;
    }
    
    ssLOG_DEBUG("Checking: " << childName << " exists");
    if(!ExistAndHasChild_LibYaml(parentNode, childName, nullable))
    {
        if(required)
        {
            if(false)
            {
                ssLOG_DEBUG("node.num_children(): " << parentNode->GetChildrenCount());
                for(int j = 0; j < parentNode->GetChildrenCount(); ++j)
                    ssLOG_DEBUG(parentNode->GetMapKeyScalarAt<std::string>(j).value_or(""));
            }
            
            ssLOG_ERROR("Required field not found: " << childName);
            return false;
        }
        return true;
    }
    
    //If type is nullable, we cannot verify it's type, so just continue
    YAML::ConstNodePtr childNode = parentNode->GetMapValueNode(childName);
    if(nullable && childNode->IsScalar() && childNode->GetScalar<StringView>().value_or("").empty())
        return true;

    if(childNode->GetType() != childType)
    {
        ssLOG_ERROR("Field type is invalid: " << childName);
        ssLOG_ERROR("Expected: " << YAML::NodeTypeToString(childType));
        ssLOG_ERROR("Found: " << YAML::NodeTypeToString(childNode->GetType()));
        return false;
    }
    
    return true;
}
    
bool runcpp2::CheckNodeRequirements_LibYaml(YAML::ConstNodePtr node, 
                                            const std::vector<NodeRequirement>& requirements)
{
    ssLOG_FUNC_DEBUG();
    
    if(!node->IsMap())
    {
        ssLOG_ERROR("Node is not a map: " << YAML::NodeTypeToString(node->GetType()));
        ssLOG_ERROR("Node is not a map");
        return false;
    }
    
    //All keys must be unique
    {
        std::unordered_set<std::string> childKeys;
        for(int i = 0; i < node->GetChildrenCount(); ++i)
        {
            YAML::ConstNodePtr keyNode = node->GetMapKeyNodeAt(i);
            if(!keyNode->IsScalar())
            {
                ssLOG_ERROR("Key must be scalar");
                return false;
            }
            
            std::string currentKey = (std::string)keyNode->GetScalar<StringView>().value_or("");
            if(childKeys.count(currentKey) != 0)
            {
                ssLOG_ERROR("Duplicate key found for: " << currentKey);
                return false;
            }
            else
                childKeys.insert(currentKey);
        }
    }
    
    for(int i = 0; i < requirements.size(); ++i)
    {
        if(!CheckNodeRequirement_LibYaml(   node, 
                                            requirements[i].Name, 
                                            requirements[i].NodeType_LibYaml, 
                                            requirements[i].Required, 
                                            requirements[i].Nullable))
        {
            return false;
        }
    }
    
    return true;
}

bool runcpp2::GetParsableInfo(const std::string& contentToParse, std::string& outParsableInfo)
{
    ssLOG_FUNC_DEBUG();

    std::string source = contentToParse;
    
    //Remove all \r characters
    for(int i = 0; i < source.size(); i++)
    {
        if(source[i] == '\r')
        {
            source.erase(i, 1);
            --i;
        }
    }
    
    const std::string prefix = "runcpp2";
    
    std::string currentLine;
    
    bool insideCommentToParse = false;
    bool singleLineComments = false;
    bool preceedingSpace = false;
    
    bool contentReadyToParse = false;
    
    auto resetPrefixState = [&]()
    {
        //Clear values
        insideCommentToParse = false;
        singleLineComments = false;
        preceedingSpace = false;
        outParsableInfo.clear();
        contentReadyToParse = false;
    };
    
    auto checkFinishedGettingParsableContent = [&]() -> bool
    {
        if(singleLineComments)
        {
            //Check for end of continuous section
            if( currentLine.size() < 2 || 
                currentLine.at(0) != '/' || 
                currentLine.at(1) != '/')
            {
                contentReadyToParse = true;
                return true;
            }
            
            if(preceedingSpace && currentLine.size() < 3)
            {
                ssLOG_ERROR("Inconsistent spacing for single line comment");
                resetPrefixState();
                return false;
            }
        }
        else
        {
            //Check for closing */
            if( currentLine.size() >= 2 &&
                currentLine.at(currentLine.size() - 1) == '/' &&
                currentLine.at(currentLine.size() - 2) == '*')
            {
                currentLine.erase(currentLine.size() - 2, 2);
                TrimRight(currentLine);
                outParsableInfo += currentLine;
                contentReadyToParse = true;
                return true;
            }
        }
        
        return false;
    };
    
    for(int i = 0; i < source.size(); i++)
    {
        //Just append character if not newline
        if(source[i] != '\n')
            currentLine.push_back(source[i]);
        //Newline is reached, parse the current line
        else
        {
            //Try to find prefix that indicates the content inside the comment for parsing
            if(!insideCommentToParse)
            {
                Trim(currentLine);
                
                if(currentLine == "// " + prefix || currentLine == "//" + prefix)
                {
                    insideCommentToParse = true;
                    singleLineComments = true;
                    preceedingSpace = (currentLine + prefix).at(2) == ' ';
                    currentLine.clear();
                    continue;
                }
                
                if(currentLine == "/* " + prefix || currentLine == "/*" + prefix)
                {
                    insideCommentToParse = true;
                    singleLineComments = false;
                    preceedingSpace = false;
                    currentLine.clear();
                    continue;
                }
                
                currentLine.clear();
            }
            //Parse the content
            else
            {
                if(singleLineComments)
                {
                    Trim(currentLine);
                    
                    if(checkFinishedGettingParsableContent())
                        break;

                    currentLine.erase(0, (preceedingSpace ? 3 : 2));
                    currentLine += '\n';
                    outParsableInfo += currentLine;
                }
                //If inside block comment
                else
                {
                    TrimRight(currentLine);
                    
                    if(checkFinishedGettingParsableContent())
                        break;
                    
                    currentLine += '\n';
                    outParsableInfo += currentLine;
                }
                
                currentLine.clear();
            }
        }
        
        //Special case for last character/line
        if(i == source.size() - 1 && insideCommentToParse)
        {
            TrimRight(currentLine);
            
            if(singleLineComments)
            {
                if(checkFinishedGettingParsableContent())
                    break;
                
                currentLine.erase(0, (preceedingSpace ? 3 : 2));
                outParsableInfo += currentLine;
                contentReadyToParse = true;
            }
            else
            {
                if(checkFinishedGettingParsableContent())
                    break;
                
                ssLOG_ERROR("Missing closing */ in block comment");
                resetPrefixState();
                break;
            }
        }
    }
    
    if(!contentReadyToParse)
        outParsableInfo.clear();
    
    return true;
}

bool runcpp2::MergeYAML_NodeChildren_LibYaml(   YAML::NodePtr nodeToMergeFrom, 
                                                YAML::NodePtr nodeToMergeTo,
                                                YAML::ResourceHandle& yamlResouce)
{
    ssLOG_FUNC_DEBUG();
    
    if(!nodeToMergeFrom->IsMap() || !nodeToMergeTo->IsMap())
    {
        ssLOG_ERROR("Merge node is not map");
        return false;
    }
    
    for(int i = 0; i < nodeToMergeFrom->GetChildrenCount(); ++i)
    {
        std::string key = nodeToMergeFrom->GetMapKeyScalarAt<std::string>(i).DS_TRY_ACT(return false);
        
        if(!ExistAndHasChild_LibYaml(nodeToMergeTo, key, true))
        {
            YAML::NodePtr fromNode = nodeToMergeFrom->GetMapValueNodeAt(i);
            fromNode->CloneToMapChild(key, nodeToMergeTo, yamlResouce).DS_TRY_ACT(return false);
        }
    }
    
    return true;
}

bool runcpp2::ExistAndHasChild_LibYaml( runcpp2::YAML::ConstNodePtr node, 
                                        const std::string& childName,
                                        bool nullable)
{
    ssLOG_FUNC_DEBUG();
    
    if(node->GetChildrenCount() > 0 && node->IsMap() && node->HasMapKey(childName))
    {
        YAML::ConstNodePtr mapVal = node->GetMapValueNode(childName);
        if(!mapVal)
            return false;
        
        if(!nullable && mapVal->IsScalar() && mapVal->GetScalar<StringView>().value_or("").empty())
            return false;
        
        return true;
    }
    
    return false;
}

//TODO: Replace with string escape in libyaml wrapper
std::string runcpp2::GetEscapedYAMLString(const std::string& input)
{
    std::string output = "\"";
    
    for(char c : input)
    {
        switch(c)
        {
            case '\\': output += "\\\\"; break;
            case '\"': output += "\\\""; break;
            case '\'': output += "\\\'"; break;
            case '\n': output += "\\n"; break;
            case '\t': output += "\\t"; break;
            default: output += c;
        }
    }
    
    output += "\"";
    return output;
}

bool runcpp2::ParseIncludes(const std::string& line, std::string& outIncludePath)
{
    //Skip if not an include line
    if(line.find("#include") == std::string::npos)
        return false;

    size_t firstQuote = line.find('\"');
    size_t firstBracket = line.find('<');
    
    //Skip if no valid include format found
    if(firstQuote == std::string::npos && firstBracket == std::string::npos)
        return false;

    bool isQuoted = firstQuote != std::string::npos && 
                    (firstBracket == std::string::npos || firstQuote < firstBracket);

    size_t start = isQuoted ? firstQuote + 1 : firstBracket + 1;
    size_t end = line.find(isQuoted ? '\"' : '>', start);
    
    if(end == std::string::npos)
        return false;

    outIncludePath = line.substr(start, end - start);
    return true;
}
