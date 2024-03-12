#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/StringUtil.hpp"

#include "ssLogger/ssLog.hpp"

runcpp2::NodeRequirement::NodeRequirement() : Name(""),
                                                        NodeType(YAML::NodeType::Null),
                                                        Required(false),
                                                        Nullable(true)
{
}
    
runcpp2::NodeRequirement::NodeRequirement( const std::string& name, 
                                            YAML::NodeType::value nodeType, 
                                            bool required,
                                            bool nullable) :    Name(name), 
                                                                NodeType(nodeType), 
                                                                Required(required), 
                                                                Nullable(nullable)
{}
    
bool runcpp2::CheckNodeRequirements(YAML::Node& node, 
                                    const std::vector<NodeRequirement>& requirements)
{
    for(int i = 0; i < requirements.size(); ++i)
    {
        if(!node[requirements[i].Name])
        {
            if(requirements[i].Required)
            {
                ssLOG_ERROR("Required field not found: " << requirements[i].Name);
                return false;
            }
            continue;
        }
        
        if(node[requirements[i].Name].Type() != requirements[i].NodeType)
        {
            if( requirements[i].Nullable && 
                node[requirements[i].Name].Type() == YAML::NodeType::Null)
            {
                continue;
            }
            
            ssLOG_ERROR("Field type is invalid: " << requirements[i].Name);
            ssLOG_ERROR("Expected: " << requirements[i].NodeType);
            ssLOG_ERROR("Found: " << node[requirements[i].Name].Type());
            return false;
        }
    }
    return true;
}

bool runcpp2::GetParsableInfo(const std::string& contentToParse, std::string& outParsableInfo)
{
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