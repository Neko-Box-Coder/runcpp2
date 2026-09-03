#ifndef RUNCPP2_RESOLVE_IMPORT_HPP
#define RUNCPP2_RESOLVE_IMPORT_HPP

#include "runcpp2/ParseUtil.hpp"
#include "runcpp2/ParameterUtil.hpp"

namespace runcpp2
{
    //TODO: We need to add import map to invalidate cache when the import yaml changes
    template<typename ImportType> 
    DS::Result<ImportType> 
    ResolveImport(  runcpp2::YAML::NodePtr currentNode, 
                    const ghc::filesystem::path& yamlDir,
                    runcpp2::YAML::ResourceHandle& currentYamlResources,
                    std::unordered_map<std::string, std::vector<std::string>> subMap,
                    const std::unordered_map<std::string, std::string>& inputParameters,
                    const std::vector<YAML::ConstNodePtr>& excludedNodes)
    {
        using namespace runcpp2;
        
        ssLOG_FUNC_INFO();
        
        std::stack<ghc::filesystem::path> pathsToImport;
        ImportType importObj = {};
        
        //Resovle parameters and variables first, before importing
        ParseParametersAndVariables(importObj, currentNode).DS_TRY();
        ApplyParametersAndVariables(importObj, 
                                    currentNode, 
                                    currentYamlResources, 
                                    subMap,
                                    inputParameters,
                                    excludedNodes).DS_TRY();
        
        ghc::filesystem::path currentImportFilePath = yamlDir / "Stub";
        while(runcpp2::ExistAndHasChild(currentNode, "Import") || !pathsToImport.empty())
        {
            //If we import field, we should deal with it instead
            if(runcpp2::ExistAndHasChild(currentNode, "Import"))
            {
                const YAML::ConstNodePtr importNode = currentNode->GetMapValueNode("Import");
                if( importNode->GetType() != YAML::NodeType::Scalar && 
                    importNode->GetType() != YAML::NodeType::Sequence)
                {
                    return DS_ERROR_MSG("Import must be a path or sequence of paths of YAML file(s)");
                }
                
                if(importNode->GetType() == YAML::NodeType::Scalar)
                {
                    std::string importPath = importNode->GetScalar<std::string>().DS_TRY();
                    pathsToImport.push(currentImportFilePath.parent_path() / importPath);
                }
                else
                {
                    if(importNode->GetChildrenCount() == 0)
                        return DS_ERROR_MSG("An import sequence cannot be an empty");
                    
                    for(int i = 0; i < importNode->GetChildrenCount(); ++i)
                    {
                        if(importNode->GetSequenceChildNode(i)->GetType() != YAML::NodeType::Scalar)
                            return DS_ERROR_MSG("It must be a sequence of paths");
                        
                        std::string importPath = importNode ->GetSequenceChildScalar<std::string>(i)
                                                            .DS_TRY();
                        pathsToImport.push(yamlDir / importPath);
                    }
                }
            }
            
            currentImportFilePath = pathsToImport.top();
            pathsToImport.pop();
            
            std::error_code ec;
            if(!ghc::filesystem::exists(currentImportFilePath, ec))
                return DS_ERROR_MSG("Import path doesn't exist: " + currentImportFilePath.string());
            
            //Read import file
            std::stringstream buffer;
            {
                std::ifstream importFile(currentImportFilePath);
                if(!importFile)
                {
                    return DS_ERROR_MSG("Failed to open import file: " + 
                                        DS_STR(currentImportFilePath));
                }
                buffer << importFile.rdbuf();
            }
            
            YAML::ResourceHandle yamlResources;
            DEFER { YAML::FreeYAMLResource(yamlResources); };
            
            std::vector<YAML::NodePtr> yamlRootNodes = YAML::ParseYAML( buffer.str(), 
                                                                        yamlResources).DS_TRY();
            DS_ASSERT_FALSE(yamlRootNodes.empty());
            for(int i = 0; i < yamlRootNodes.size(); ++i)
            {
                YAML::ResolveAnchors(yamlRootNodes[i]).DS_TRY();
                YAML::NodePtr importNode = yamlRootNodes[i];
                
                ImportType tempImportObj = {};
                
                //Perform parameter & variable parsing if the imported stuff has parameters/variables
                ParseParametersAndVariables(tempImportObj, importNode).DS_TRY();
                
                std::unordered_map<std::string, std::vector<std::string>> subMapCopy = subMap;
                
                //Modify the import yaml node
                ApplyParametersAndVariables(tempImportObj, 
                                            importNode, 
                                            currentYamlResources, 
                                            subMapCopy,
                                            inputParameters,
                                            {}).DS_TRY();
                
                MergeYamlNode(importNode, currentNode, currentYamlResources).DS_TRY();
                
                //If the import doesn't have import, remove the current import
                if(!ExistAndHasChild(importNode, "Import"))
                {
                    currentNode->RemoveMapChild("Import").DS_TRY();
                }
            }
        } //while(runcpp2::ExistAndHasChild(currentNode, "Import") || !pathsToImport.empty())
        
        return importObj;
    }
}

#endif
