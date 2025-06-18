#include "runcpp2/Data/DependencySource.hpp"
#include "runcpp2/Data/ParseCommon.hpp"
#include "runcpp2/ParseUtil.hpp"
#include "ssLogger/ssLog.hpp"

bool runcpp2::Data::DependencySource::ParseYAML_Node(ryml::ConstNodeRef node)
{
    INTERNAL_RUNCPP2_SAFE_START();
    
    if(ExistAndHasChild(node, "ImportPath"))
    {
        std::string importPathStr;
        node["ImportPath"] >> importPathStr;
        ImportPath = importPathStr;
        
        if(ImportPath.is_absolute())
        {
            ssLOG_ERROR("DependencySource: ImportPath must be relative: " << ImportPath.string());
            return false;
        }
    }
    
    if(ExistAndHasChild(node, "Git"))
    {
        if(ExistAndHasChild(node, "Local"))
        {
            ssLOG_ERROR("DependencySource: Both Git and Local sources found");
            return false;
        }
        
        GitSource gitSource;
        ryml::ConstNodeRef gitNode = node["Git"];
        if(!gitSource.ParseYAML_Node(gitNode))
            return false;
        Source = gitSource;
        return true;
    }
    else if(ExistAndHasChild(node, "Local"))
    {
        if(ExistAndHasChild(node, "Git"))
        {
            ssLOG_ERROR("DependencySource: Both Git and Local sources found");
            return false;
        }
        
        LocalSource localSource;
        ryml::ConstNodeRef localNode = node["Local"];
        if(!localSource.ParseYAML_Node(localNode))
            return false;
        Source = localSource;
        return true;
    }
    //If no source is found, we need to check if it's an imported source. 
    //If so, we assume it's a local source with path "./"
    else if(!ImportPath.empty())
    {
        LocalSource localSource;
        localSource.Path = "./";
        Source = localSource;
        return true;
    }
    
    ssLOG_ERROR("DependencySource: Neither Git nor Local source found");
    return false;
    
    INTERNAL_RUNCPP2_SAFE_CATCH_RETURN(false);
}

std::string runcpp2::Data::DependencySource::ToString(std::string indentation) const
{   
    std::string out;
    if(!ImportPath.empty())
        out += indentation + "ImportPath: " + GetEscapedYAMLString(ImportPath.string()) + "\n";
    
    if(mpark::get_if<GitSource>(&Source))
    {
        const GitSource* git = mpark::get_if<GitSource>(&Source);
        out += git->ToString(indentation);
    }
    else if(mpark::get_if<LocalSource>(&Source))
    {
        const LocalSource* local = mpark::get_if<LocalSource>(&Source);
        out += local->ToString(indentation);
    }
    else
    {
        ssLOG_ERROR("Invalid DependencySource type");
        return "";
    }
    return out;
}

bool runcpp2::Data::DependencySource::Equals(const DependencySource& other) const
{
    if(ImportPath != other.ImportPath)
        return false;
        
    if(mpark::get_if<GitSource>(&Source))
    {
        if(mpark::get_if<GitSource>(&other.Source))
        {
            const GitSource* git = mpark::get_if<GitSource>(&Source);
            const GitSource* otherGit = mpark::get_if<GitSource>(&other.Source);
            return git->Equals(*otherGit);
        }
        else
            return false;
    }
    else if(mpark::get_if<LocalSource>(&Source))
    {
        if(mpark::get_if<LocalSource>(&other.Source))
        {
            const LocalSource* local = mpark::get_if<LocalSource>(&Source);
            const LocalSource* otherLocal = mpark::get_if<LocalSource>(&other.Source);
            return local->Equals(*otherLocal);
        }
        else
            return false;
    }
    
    ssLOG_ERROR("Invalid DependencySource type");
    return false;
}
