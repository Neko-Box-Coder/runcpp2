#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/StringUtil.hpp"

#include "ssLogger/ssLog.hpp"

runcpp2::NodeRequirement::NodeRequirement() :   Name(""),
                                                NodeType(),
                                                Required(false),
                                                Nullable(true)
{
}
    
runcpp2::NodeRequirement::NodeRequirement(  const std::string& name, 
                                            ryml::NodeType nodeType, 
                                            bool required,
                                            bool nullable) :    Name(name), 
                                                                NodeType(nodeType), 
                                                                Required(required), 
                                                                Nullable(nullable)
{}

bool runcpp2::CheckNodeRequirements(ryml::ConstNodeRef& node, 
                                    const std::vector<NodeRequirement>& requirements)
{
    ssLOG_FUNC_DEBUG();
    
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(node.invalid())
    {
        ssLOG_ERROR("Node is invalid");
        return false;
    }
    
    if( !INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::MAP) && 
        !INTERNAL_RUNCPP2_BIT_CONTANTS(node.type().type, ryml::NodeType_e::KEYVAL))
    {
        ssLOG_ERROR("Node is not a map: " << node.type().type_str());
        return false;
    }
    
    for(int i = 0; i < requirements.size(); ++i)
    {
        ssLOG_DEBUG("Checking: " << requirements[i].Name << " exists");
        
        if(!ExistAndHasChild(node, requirements[i].Name, requirements[i].Nullable))
        {
            if(requirements[i].Required)
            {
                if(0)
                {
                    ssLOG_DEBUG("node.num_children(): " << node.num_children());
                    
                    for(int j = 0; j < node.num_children(); ++j)
                        ssLOG_DEBUG(node[j].key());
                }
                
                ssLOG_ERROR("Required field not found: " << requirements[i].Name);
                return false;
            }
            continue;
        }
        
        //If type is nullable, we cannot verify it's type, so just continue
        if( requirements[i].Nullable && 
            node[requirements[i].Name.c_str()].is_keyval() &&
            node[requirements[i].Name.c_str()].val_is_null())
        {
            continue;
        }
        
        //Debug prints
        if(0)
        {
            ssLOG_DEBUG("Checking: " << requirements[i].Name << " type");
            ssLOG_DEBUG("node[" << requirements[i].Name << "].is_keyval(): " <<
                        node[requirements[i].Name.c_str()].is_keyval());
            
            if(node[requirements[i].Name.c_str()].is_keyval())
            {
                ssLOG_DEBUG("node[" << requirements[i].Name << "].val_is_null(): " <<
                            node[requirements[i].Name.c_str()].val_is_null());
            }
            ssLOG_DEBUG("node[" << requirements[i].Name << "].type().type: " << 
                        static_cast<int>(node[requirements[i].Name.c_str()].type().type));
            ssLOG_DEBUG("requirements[" << i << "].NodeType.type: " << 
                        static_cast<int>(requirements[i].NodeType.type));
        }
        
        if(!INTERNAL_RUNCPP2_BIT_CONTANTS(  node[requirements[i].Name.c_str()].type().type, 
                                            requirements[i].NodeType.type))
        {
            ssLOG_ERROR("Field type is invalid: " << requirements[i].Name);
            ssLOG_ERROR("Expected: " << c4::yml::NodeType::type_str(requirements[i].NodeType));
            ssLOG_ERROR("Found: " << node[requirements[i].Name.c_str()].type_str());
            return false;
        }
    }
    return true;
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
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

bool runcpp2::ResolveYAML_Stream(   ryml::Tree& rootTree, 
                                    ryml::ConstNodeRef& outRootNode)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    //Resolve the merge keys in the yaml first
    rootTree.resolve();
    
    std::string temp;
    
    if(rootTree.rootref().is_stream())
    {
        ssLOG_DEBUG("rootTree.rootref().num_children(): " << rootTree.rootref().num_children());
        
        for(int i = 0; i < rootTree.rootref().num_children(); ++i)
        {
            if( rootTree.rootref()[i].num_children() > 0 &&
                rootTree.rootref()[i].is_map())
            {
                outRootNode = rootTree.rootref()[i];
                ssLOG_DEBUG( "rootTree.rootref()[" << i << "].num_children(): " << 
                            rootTree.rootref()[i].num_children());
                
                return true;
            }
        }
        
        ssLOG_ERROR("Invalid format");
        return false;
    }
    else
        outRootNode = rootTree.rootref();
    
    return true;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

bool runcpp2::ExistAndHasChild( const ryml::ConstNodeRef& node, 
                                const std::string& childName,
                                bool nullable)
{
    ssLOG_FUNC_DEBUG();

    if(node.num_children() > 0 && node.has_child(childName.c_str()))
    {
        if(!nullable && node[childName.c_str()].has_val())
        {
            if(node[childName.c_str()].val_is_null() || node[childName.c_str()].val().empty())
                return false;
        }
        
        return true;
    }
    
    return false;
}

std::string runcpp2::GetValue(ryml::ConstNodeRef node)
{
    return std::string(node.val().str, node.val().len);
}

std::string runcpp2::GetKey(ryml::ConstNodeRef node)
{
    return std::string(node.key().str, node.key().len);
}

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
