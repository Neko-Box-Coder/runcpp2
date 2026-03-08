#ifndef RUNCPP2_LIB_YAML_WRAPPER_HPP
#define RUNCPP2_LIB_YAML_WRAPPER_HPP

#include "DSResult/DSResult.hpp"
#include "mpark/variant.hpp"
#include "nonstd/string_view.hpp"
#include "yaml.h"
#include "ssLogger/ssLog.hpp"

#include <stack>
#include <utility>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <exception>
#include <limits>
#include <cctype>
#include <cstdint>

typedef yaml_event_s yaml_event_t;
typedef yaml_parser_s yaml_parser_t;

#if RUNCPP2_YAML_PRINT_PARSE
    #include <cstdio>
    #define RUNCPP2_YAML_PRINT(...) printf(__VA_ARGS__)
#else
    #define RUNCPP2_YAML_PRINT(...) do{} while(false)
#endif

namespace runcpp2
{
    using StringView = nonstd::string_view;
    namespace YAML
    {
        struct Node;
        using ScalarValue = StringView;
        struct Alias;
        using NodePtr = std::shared_ptr<Node>;
        using Sequence = std::vector<NodePtr>;
        struct OrderedMap;
        using NodeValue = mpark::variant<ScalarValue, Alias, Sequence, OrderedMap>;
    }
}

namespace
{
    std::string EscapeString(const runcpp2::StringView string);
    std::string ScalarToString(const runcpp2::YAML::ScalarValue& scalar);
    std::string AliasToString(const runcpp2::YAML::Alias& alias);
    
    template<bool pushToStack, typename YamlValueType>
    DS::Result<void> AddValueToMap( std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                            int>>& nodeValCountStack,
                                    const YamlValueType& newVal,
                                    const yaml_event_t& event);
    
    DS::Result<void> ParseHandleMap(yaml_event_t& event, 
                                    std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                            int>>& nodeValCountStack);
    
    DS::Result<void> ParseHandleSequence(   yaml_event_t& event, 
                                            std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                                    int>>& nodeValCountStack);
    
    DS::Result<void> ParseHandleScalar( yaml_event_t& event, 
                                        std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                                int>>& nodeValCountStack);
    
    DS::Result<void> ParseHandleAlias(  yaml_event_t& event, 
                                        std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                                int>>& nodeValCountStack);
    
    DS::Result<void> CopyVariant(runcpp2::YAML::NodeValue& dest, const runcpp2::YAML::NodeValue& src);
    
    DS::Result<void> UpdateParentsRecursively(runcpp2::YAML::Node& currentNode);
    
    //NOTE: This doesn't update OrderedMap.StringMap
    DS::Result<void> ResolveMergeKey(   runcpp2::YAML::OrderedMap& currentMap, 
                                        int mergeKeyIndex,
                                        const std::unordered_map<   runcpp2::StringView, 
                                                                    runcpp2::YAML::Node*>& anchors);
}

namespace runcpp2
{
    namespace YAML
    {
        enum class NodeType
        {
            Scalar, 
            Alias,
            Sequence, 
            Map
        };
        
        inline StringView NodeTypeToString(NodeType type);
    
        using NodePtr = std::shared_ptr<Node>;
        using ConstNodePtr = std::shared_ptr<const Node>;
        
        struct ReadWriteBuffer
        {
            std::vector<std::shared_ptr<yaml_event_t>> ReadData = {};
            //NOTE: Needs to be unique_ptr due to small string optimizations
            std::vector<std::unique_ptr<std::string>> WriteData = {};
            
            inline std::string& CreateString()
            {
                WriteData.emplace_back(std::unique_ptr<std::string>(new std::string()));
                return *WriteData.back().get();
            }
            
            inline void StoreString(const std::string& str)
            {
                CreateString() = str;
            }
        };
        
        using ResourceHandle = std::pair<yaml_parser_t*, ReadWriteBuffer>;
        
        inline NodePtr CreateNodePtr() { return std::make_shared<Node>(); }
        
        //NOTE: all parameters must be alive while using the node
        inline DS::Result<std::vector<NodePtr>> ParseYAML(  StringView yamlString, 
                                                            ResourceHandle& outResource);
    
        inline DS::Result<void> ResolveAnchors(NodePtr rootNode);
        
        inline DS::Result<void> FreeYAMLResource(ResourceHandle& resourceHandleToFree);
        
        struct Alias
        {
            StringView Value;
        };
        
        using Sequence = std::vector<NodePtr>;
        using ScalarValue = StringView;
        
        struct OrderedMap
        {
            std::vector<NodePtr> InsertedKeys;
            std::unordered_map<NodePtr, NodePtr> Map;
            std::unordered_map<ScalarValue, NodePtr> StringMap;
        };
        
        using NodeValue = mpark::variant<ScalarValue, Alias, Sequence, OrderedMap>;
        
        struct Node
        {
            NodeValue Value = "";
            StringView Anchor = "";
            int LineNumber = -1;
            Node* Parent = nullptr;
            
            //Reading
            inline NodeType GetType() const { return (NodeType)Value.index(); }
            
            inline bool IsAlias() const { return mpark::holds_alternative<Alias>(Value); };
            inline bool IsScalar() const { return mpark::holds_alternative<ScalarValue>(Value); };
            inline bool IsSequence() const { return mpark::holds_alternative<Sequence>(Value); };
            inline bool IsMap() const { return mpark::holds_alternative<OrderedMap>(Value); };
            
            template<typename T>
            inline DS::Result<T> GetAlias() const;
            
            template<typename T>
            inline DS::Result<T> GetScalar() const;

            inline NodePtr GetSequenceChildNode(uint32_t index);
            inline ConstNodePtr GetSequenceChildNode(uint32_t index) const;
            
            template<typename T>
            inline DS::Result<T> GetSequenceChildAlias(uint32_t index) const;
            
            template<typename T>
            inline DS::Result<T> GetSequenceChildScalar(uint32_t index) const;
            
            inline bool HasMapKey(StringView key) const;
            inline NodePtr GetMapValueNode(StringView key);
            inline ConstNodePtr GetMapValueNode(StringView key) const;
            
            template<typename T>
            inline DS::Result<T> GetMapValueAlias(StringView key) const;
            
            template<typename T>
            inline DS::Result<T> GetMapValueScalar(StringView key) const;
            
            inline NodePtr GetMapKeyNodeAt(uint32_t index);
            inline ConstNodePtr GetMapKeyNodeAt(uint32_t index) const;
            
            template<typename T>
            inline DS::Result<T> GetMapKeyAliasAt(uint32_t index) const;
            
            template<typename T>
            inline DS::Result<T> GetMapKeyScalarAt(uint32_t index) const;
            
            inline NodePtr GetMapValueNodeAt(uint32_t index);
            inline ConstNodePtr GetMapValueNodeAt(uint32_t index) const;
            
            template<typename T>
            inline DS::Result<T> GetMapValueAliasAt(uint32_t index) const;
            
            template<typename T>
            inline DS::Result<T> GetMapValueScalarAt(uint32_t index) const;
            
            inline uint32_t GetChildrenCount() const;
            
            //Writing
            DS::Result<NodePtr> CloneToSequenceChild(   NodePtr parentNode, 
                                                        ResourceHandle& yamlResouce) const;
            
            //TODO: Support ordering sequence child
            
            //TODO: Support erasing sequence child
            
            //NOTE: `key` will be copied
            DS::Result<NodePtr> CloneToMapChild(StringView key, 
                                                NodePtr parentNode, 
                                                ResourceHandle& yamlResouce) const;
            
            //TODO: Support ordering map child
            
            //TODO: Support erasing map child
            inline DS::Result<void> RemoveMapChild(StringView key);
            
            
            //TODO: Proper Writing
            
            //TODO: Support null (null tag and ~)?
            
            DS::Result<void> ToString(std::string& outString) const;
            
            DS::Result<NodePtr> Clone(bool shallow, ResourceHandle& yamlResouce) const;
        };
        
        //==================================================
        //Node functions inline implementations
        //==================================================
        
        inline StringView NodeTypeToString(NodeType type)
        {
            switch(type)
            {
                case NodeType::Scalar:
                    return "Scalar";
                case NodeType::Alias:
                    return "Alias";
                case NodeType::Sequence:
                    return "Sequence";
                case NodeType::Map:
                    return "Map";
                default:
                    return "";
            }
        }
        
        inline DS::Result<std::vector<NodePtr>> ParseYAML(  StringView yamlString, 
                                                            ResourceHandle& outResource)
        {
            ssLOG_FUNC_DEBUG();
            std::vector<NodePtr> rootNodes;
            
            outResource.first = nullptr;
            outResource.first = new yaml_parser_t();
            DS_ASSERT_TRUE(outResource.first != nullptr);
            yaml_parser_t& parser = *outResource.first;
            ReadWriteBuffer& readBuffer = outResource.second;
            
            DS_ASSERT_EQ(yaml_parser_initialize(&parser), 1);
            
            yaml_parser_set_input_string(   &parser, 
                                            (const unsigned char*)yamlString.data(), 
                                            yamlString.size());
            
            std::stack<std::pair<Node*, int>> nodeValCountStack;
            
            while(true)
            {
                readBuffer.ReadData.emplace_back(std::shared_ptr<yaml_event_t>(new yaml_event_t()));
                yaml_event_t& event = *readBuffer.ReadData.back().get();
                
                if(!yaml_parser_parse(&parser, &event))
                {
                    if(parser.problem_mark.line || parser.problem_mark.column)
                    {
                        return DS_ERROR_MSG(DS_STR("Parsing failed: ") + parser.problem + " on line " +
                                            DS_STR(parser.problem_mark.line + 1) + ", column" +
                                            DS_STR(parser.problem_mark.column + 1));
                    }
                    else
                        return DS_ERROR_MSG(DS_STR("Parsing failed: ") + parser.problem);
                }
                
                switch(event.type)
                {
                    case YAML_NO_EVENT:
                        ssLOG_DEBUG("???\n");
                        break;
                    case YAML_STREAM_START_EVENT:
                        ssLOG_DEBUG("+STR\n");
                        break;
                    case YAML_STREAM_END_EVENT:
                        ssLOG_DEBUG("-STR\n");
                        break;
                    case YAML_DOCUMENT_START_EVENT:
                        if(!event.data.document_start.implicit)
                            ssLOG_DEBUG("+DOC ---");
                        else
                            ssLOG_DEBUG("+DOC");
                        rootNodes.push_back(CreateNodePtr());
                        nodeValCountStack.push(std::pair<Node*, int>(rootNodes.back().get(), -1));
                        break;
                    case YAML_DOCUMENT_END_EVENT:
                        if(!event.data.document_end.implicit)
                            ssLOG_DEBUG("-DOC ...");
                        else
                            ssLOG_DEBUG("-DOC");
                        
                        //NOTE: Node is popped either by map end or sequence end event, 
                        //      hence it should be empty.
                        DS_ASSERT_TRUE(nodeValCountStack.empty());
                        break;
                    case YAML_MAPPING_START_EVENT:
                        DS_UNWRAP_VOID(ParseHandleMap(event, nodeValCountStack));
                        break;
                    case YAML_MAPPING_END_EVENT:
                        nodeValCountStack.pop();
                        ssLOG_DEBUG("-MAP");
                        break;
                    case YAML_SEQUENCE_START_EVENT:
                        DS_UNWRAP_VOID(ParseHandleSequence(event, nodeValCountStack));
                        break;
                    case YAML_SEQUENCE_END_EVENT:
                        ssLOG_DEBUG("-SEQ");
                        nodeValCountStack.pop();
                        break;
                    case YAML_SCALAR_EVENT:
                        DS_UNWRAP_VOID(ParseHandleScalar(event, nodeValCountStack));
                        break;
                    case YAML_ALIAS_EVENT:
                        DS_UNWRAP_VOID(ParseHandleAlias(event, nodeValCountStack));
                        break;
                    default:
                        return DS_ERROR_MSG(DS_STR("Unrecognize event type: ") + 
                                            DS_STR((int)event.type));
                } //switch(event.type)
                
                if(event.type == YAML_STREAM_END_EVENT)
                    break;
            } //while(true)
            
            return rootNodes;
        }
        
        inline DS::Result<void> ResolveAnchors(NodePtr rootNode)
        {
            DS_ASSERT_FALSE(mpark::holds_alternative<ScalarValue>(rootNode->Value) || 
                            mpark::holds_alternative<Alias>(rootNode->Value));
            
            std::stack<std::pair<Node*, int>> nodeValCountStack;
            nodeValCountStack.push(std::pair<Node*, int>(rootNode.get(), 0));
            
            std::unordered_map<StringView, Node*> anchors;
            std::vector<Node*> mapsToUpdateStringMapKeys;
            
            while(!nodeValCountStack.empty())
            {
                Node& currentNode = *nodeValCountStack.top().first;
                int visitCount = nodeValCountStack.top().second++;
                
                //We visited once already, all the children are processed.
                if(visitCount >= 1)
                {
                    nodeValCountStack.pop();
                    continue;
                }
                
                if(mpark::holds_alternative<Alias>(currentNode.Value))
                {
                    StringView aliasValue = mpark::get_if<Alias>(&currentNode.Value)->Value;
                    DS_ASSERT_FALSE(aliasValue.empty());
                    if(anchors.count(aliasValue) == 0)
                    {
                        return DS_ERROR_MSG("Cannot find anchor value: " + DS_STR(aliasValue) + 
                                            " on line " + DS_STR(currentNode.LineNumber));
                    }
                    
                    //Check parents don't have the anchor we want
                    Node* parent = &currentNode;
                    while(parent != nullptr)
                    {
                        if(parent->Anchor == aliasValue)
                        {
                            return DS_ERROR_MSG("Cannot alias parent anchor: " + 
                                                DS_STR(aliasValue) + 
                                                " on line " + DS_STR(currentNode.LineNumber) + 
                                                " with parent on line " + 
                                                DS_STR(parent->LineNumber));
                        }
                        parent = parent->Parent;
                    }
                    
                    //Resolve alias with the anchor we found
                    DS_UNWRAP_VOID(CopyVariant(currentNode.Value, anchors[aliasValue]->Value));
                    
                    //Check if parent is map, remember to update the string map
                    if( currentNode.Parent != nullptr && 
                        mpark::holds_alternative<OrderedMap>(currentNode.Parent->Value))
                    {
                        mapsToUpdateStringMapKeys.push_back(currentNode.Parent);
                    }
                    
                    //The parent entry for this node is correct, but not the children, update them.
                    DS_UNWRAP_VOID(UpdateParentsRecursively(currentNode));
                }
                else if(mpark::holds_alternative<ScalarValue>(currentNode.Value))
                    ++(nodeValCountStack.top().second);
                else if(mpark::holds_alternative<Sequence>(currentNode.Value))
                {
                    Sequence& sequence = *mpark::get_if<Sequence>(&currentNode.Value);
                    for(int i = sequence.size() - 1; i >= 0; --i)
                        nodeValCountStack.push(std::pair<Node*, int>(sequence[i].get(), 0));
                }
                else if(mpark::holds_alternative<OrderedMap>(currentNode.Value))
                {
                    OrderedMap& orderedMap = *mpark::get_if<OrderedMap>(&currentNode.Value);
                    bool hasMergeKeys = false;
                    
                    //Resolve merge keys first if any
                    for(int i = 0; i < orderedMap.InsertedKeys.size(); ++i)
                    {
                        NodePtr currentKey = orderedMap.InsertedKeys[i];
                        if(!mpark::holds_alternative<Alias>(currentKey->Value))
                            continue;
                        
                        //TODO: Add support for alias map key
                        if(mpark::get_if<Alias>(&currentKey->Value)->Value != "<<")
                            return DS_ERROR_MSG("Alias key must be a merge key");
                    
                        //Move all the alias children to current map
                        DS_UNWRAP_VOID(ResolveMergeKey(orderedMap, i, anchors));
                        
                        hasMergeKeys = true;
                    }
                    
                    //Need to udpate the parent entry for the children coming from merge keys, if any.
                    //Then we need to update the string map.
                    if(hasMergeKeys)
                    {
                        DS_UNWRAP_VOID(UpdateParentsRecursively(currentNode));
                        mapsToUpdateStringMapKeys.push_back(&currentNode);
                    }
                    
                    //Push all the children to the stack to be resolved
                    for(int i = orderedMap.InsertedKeys.size() - 1; i >= 0; --i)
                    {
                        NodePtr currentKey = orderedMap.InsertedKeys[i];
                        nodeValCountStack.push(std::pair<Node*, int>(orderedMap.Map[currentKey].get(), 0));
                        nodeValCountStack.push(std::pair<Node*, int>(currentKey.get(), 0));
                    }
                }
                else
                    return DS_ERROR_MSG("Invalid type");
                
                if(!currentNode.Anchor.empty())
                    anchors[currentNode.Anchor] = &currentNode;
            } //while(!nodeValCountStack.empty())
            
            //Update string map for maps in mapsToUpdateStringMapKeys for any missing scalar keys
            for(int i = 0; i < mapsToUpdateStringMapKeys.size(); ++i)
            {
                DS_ASSERT_TRUE(mpark::holds_alternative<OrderedMap>(mapsToUpdateStringMapKeys[i]->Value));
                OrderedMap& currentMap = *mpark::get_if<OrderedMap>(&mapsToUpdateStringMapKeys[i]->Value);
                
                for(int j = 0; j < currentMap.InsertedKeys.size(); ++j)
                {
                    if(mpark::holds_alternative<ScalarValue>(currentMap.InsertedKeys[j]->Value))
                    {
                        ScalarValue& curKey = *mpark::get_if<ScalarValue>(&currentMap   .InsertedKeys[j]
                                                                                        ->Value);
                        DS_ASSERT_TRUE(currentMap.Map.count(currentMap.InsertedKeys[j]) > 0);
                        if(currentMap.StringMap.count(curKey) == 0)
                            currentMap.StringMap[curKey] = currentMap.Map[currentMap.InsertedKeys[j]];
                    }
                }
            }
            
            return {};
        }
        
        inline DS::Result<void> FreeYAMLResource(ResourceHandle& resourceHandleToFree)
        {
            DS_ASSERT_TRUE(resourceHandleToFree.first != nullptr);
            
            for(int i = 0; i < resourceHandleToFree.second.ReadData.size(); ++i)
                yaml_event_delete(resourceHandleToFree.second.ReadData[i].get());
            
            yaml_parser_delete(resourceHandleToFree.first);
            
            delete resourceHandleToFree.first;
            resourceHandleToFree.first = nullptr;
            resourceHandleToFree.second = ReadWriteBuffer();
            return {};
        }
        
        template<>
        inline DS::Result<StringView> Node::GetAlias<StringView>() const
        {
            DS_ASSERT_TRUE(IsAlias());
            return mpark::get_if<Alias>(&Value)->Value;
        }
        
        template<>
        inline DS::Result<std::string> Node::GetAlias<std::string>() const
        {
            DS_ASSERT_TRUE(IsAlias());
            return (std::string)mpark::get_if<Alias>(&Value)->Value;
        }
        
        #define INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(convertFunc) \
            do \
            { \
                try \
                { \
                    return convertFunc((std::string)*mpark::get_if<ScalarValue>(&Value)); \
                } \
                catch(std::exception& ex) \
                { \
                    return DS_ERROR_MSG(ex.what()); \
                } \
            } while(false)
        
        template<> 
        inline DS::Result<int> Node::GetScalar<int>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(std::stoi);
        }
        
        template<> 
        inline DS::Result<unsigned long> Node::GetScalar<unsigned long>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(std::stoul);
        }
        
        template<> 
        inline DS::Result<long> Node::GetScalar<long>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(std::stol);
        }
        
        template<> 
        inline DS::Result<unsigned long long> Node::GetScalar<unsigned long long>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(std::stoull);
        }
        
        template<> 
        inline DS::Result<long long> Node::GetScalar<long long>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(std::stoll);
        }
        
        template<> 
        inline DS::Result<unsigned int> Node::GetScalar<unsigned int>() const
        {
            DS_UNWRAP_DECL(unsigned long long ull, GetScalar<unsigned long long>());
            DS_ASSERT_LT_EQ(ull, std::numeric_limits<unsigned int>::max());
            return (unsigned int)ull;
        }
        
        template<> 
        inline DS::Result<float> Node::GetScalar<float>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(std::stof);
        }
        
        template<> 
        inline DS::Result<double> Node::GetScalar<double>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR(std::stod);
        }
        
        #undef INTERNAL_RUNCPP2_TRY_CONVERT_SCALAR
        
        template<> 
        inline DS::Result<bool> Node::GetScalar<bool>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            
            std::string v = (std::string)*mpark::get_if<ScalarValue>(&Value);
            for(int i = 0; i < v.size(); ++i)
                v[i] = std::tolower(v[i]);
            
            //TODO: Strict mode which only allows lowercase true and false?
            bool isTrue = v == "true" || v == "yes" || v == "on" || v == "1";
            if(isTrue)
                return true;
            else if(v == "false" || v == "no" || v == "off" || v == "0")
                return false;
            else
                return DS_ERROR_MSG("Invalid scalar value \"" + v + "\" to be converted to bool");
        }
        
        template<> 
        inline DS::Result<StringView> Node::GetScalar<StringView>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            return *mpark::get_if<ScalarValue>(&Value);
        }
        
        template<> 
        inline DS::Result<std::string> Node::GetScalar<std::string>() const
        {
            DS_ASSERT_TRUE(IsScalar());
            return (std::string)*mpark::get_if<ScalarValue>(&Value);
        }
        
        inline NodePtr Node::GetSequenceChildNode(uint32_t index)
        {
            if(!IsSequence() || index >= mpark::get_if<Sequence>(&Value)->size())
                return nullptr;
            return (*mpark::get_if<Sequence>(&Value))[index];
        }
    
        inline ConstNodePtr Node::GetSequenceChildNode(uint32_t index) const
        {
            if(!IsSequence() || index >= mpark::get_if<Sequence>(&Value)->size())
                return nullptr;
            return (*mpark::get_if<Sequence>(&Value))[index];
        }
        
        template<typename T>
        inline DS::Result<T> Node::GetSequenceChildAlias(uint32_t index) const
        {
            DS_ASSERT_TRUE(IsSequence());
            DS_ASSERT_LT(index, mpark::get_if<Sequence>(&Value)->size());
            return (*mpark::get_if<Sequence>(&Value))[index]->GetAlias<T>();
        }
        
        template<typename T>
        inline DS::Result<T> Node::GetSequenceChildScalar(uint32_t index) const
        {
            DS_ASSERT_TRUE(IsSequence());
            DS_ASSERT_LT(index, mpark::get_if<Sequence>(&Value)->size());
            return (*mpark::get_if<Sequence>(&Value))[index]->GetScalar<T>();
        }
        
        inline bool Node::HasMapKey(StringView key) const
        {
            if(!IsMap())
                return false;
            return mpark::get_if<OrderedMap>(&Value)->StringMap.count(key) > 0;
        }
        
        inline NodePtr Node::GetMapValueNode(StringView key)
        {
            if(!IsMap() || mpark::get_if<OrderedMap>(&Value)->StringMap.count(key) == 0)
                return nullptr;
            return mpark::get_if<OrderedMap>(&Value)->StringMap.at(key);
        }
        
        inline ConstNodePtr Node::GetMapValueNode(StringView key) const
        {
            if(!IsMap() || mpark::get_if<OrderedMap>(&Value)->StringMap.count(key) == 0)
                return nullptr;
            return mpark::get_if<OrderedMap>(&Value)->StringMap.at(key);
        }
    
        template<typename T>
        inline DS::Result<T> Node::GetMapValueAlias(StringView key) const
        {
            DS_ASSERT_TRUE(IsMap());
            DS_ASSERT_TRUE(HasMapKey(key));
            return mpark::get_if<OrderedMap>(&Value)->StringMap.at(key)->GetAlias<T>();
        }
        
        template<typename T>
        inline DS::Result<T> Node::GetMapValueScalar(StringView key) const
        {
            DS_ASSERT_TRUE(IsMap());
            DS_ASSERT_TRUE(HasMapKey(key));
            return mpark::get_if<OrderedMap>(&Value)->StringMap.at(key)->GetScalar<T>();
        }
    
        inline NodePtr Node::GetMapKeyNodeAt(uint32_t index)
        {
            if(!IsMap() || index >= mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size())
                return nullptr;
            return mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index];
        }
    
        inline ConstNodePtr Node::GetMapKeyNodeAt(uint32_t index) const
        {
            if(!IsMap() || index >= mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size())
                return nullptr;
            return mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index];
        }
        
        template<typename T>
        inline DS::Result<T> Node::GetMapKeyAliasAt(uint32_t index) const
        {
            DS_ASSERT_TRUE(IsMap());
            DS_ASSERT_LT(index, mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size());
            return mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index]->GetAlias<T>();
        }
        
        template<typename T>
        inline DS::Result<T> Node::GetMapKeyScalarAt(uint32_t index) const
        {
            DS_ASSERT_TRUE(IsMap());
            DS_ASSERT_LT(index, mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size());
            return mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index]->GetScalar<T>();
        }
        
        inline NodePtr Node::GetMapValueNodeAt(uint32_t index)
        {
            if(!IsMap() || index >= mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size())
                return nullptr;
            
            NodePtr keyNode = mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index];
            if(mpark::get_if<OrderedMap>(&Value)->Map.count(keyNode) == 0)
                return nullptr;
            
            return mpark::get_if<OrderedMap>(&Value)->Map.at(keyNode);
        }
        
        inline ConstNodePtr Node::GetMapValueNodeAt(uint32_t index) const
        {
            if(!IsMap() || index >= mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size())
                return nullptr;
            
            NodePtr keyNode = mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index];
            if(mpark::get_if<OrderedMap>(&Value)->Map.count(keyNode) == 0)
                return nullptr;
            
            return mpark::get_if<OrderedMap>(&Value)->Map.at(keyNode);
        }
        
        template<typename T>
        inline DS::Result<T> Node::GetMapValueAliasAt(uint32_t index) const
        {
            DS_ASSERT_TRUE(IsMap());
            DS_ASSERT_LT(index, mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size());
            
            NodePtr keyNode = mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index];
            DS_ASSERT_NOT_EQ(mpark::get_if<OrderedMap>(&Value)->Map.count(keyNode), 0);
            
            return mpark::get_if<OrderedMap>(&Value)->Map.at(keyNode)->GetAlias<T>();
        }
        
        template<typename T>
        inline DS::Result<T> Node::GetMapValueScalarAt(uint32_t index) const
        {
            DS_ASSERT_TRUE(IsMap());
            DS_ASSERT_LT(index, mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size());
            
            NodePtr keyNode = mpark::get_if<OrderedMap>(&Value)->InsertedKeys[index];
            DS_ASSERT_NOT_EQ(mpark::get_if<OrderedMap>(&Value)->Map.count(keyNode), 0);
            
            return mpark::get_if<OrderedMap>(&Value)->Map.at(keyNode)->GetScalar<T>();
        }
        
        inline uint32_t Node::GetChildrenCount() const
        {
            if(IsAlias() || IsScalar())
                return 0;
            else if(IsMap())
                return mpark::get_if<OrderedMap>(&Value)->InsertedKeys.size();
            else if(IsSequence())
                return mpark::get_if<Sequence>(&Value)->size();
            else
                return 0;
        }
        
        //TODO: Reorder the whole thing to match declaration order
        inline DS::Result<NodePtr> Node::CloneToSequenceChild(  NodePtr parentNode, 
                                                                ResourceHandle& yamlResouce) const
        {
            DS_ASSERT_TRUE(parentNode->IsSequence());
            NodePtr clonedThis = Clone(false, yamlResouce).DS_TRY();
            clonedThis->Parent = parentNode.get();
            mpark::get_if<Sequence>(&parentNode->Value)->push_back(clonedThis);
            return clonedThis;
        }
        
        inline DS::Result<NodePtr> Node::CloneToMapChild(   StringView key, 
                                                            NodePtr parentNode, 
                                                            ResourceHandle& yamlResouce) const
        {
            DS_ASSERT_TRUE(parentNode->IsMap());
            NodePtr clonedThis = Clone(false, yamlResouce).DS_TRY();
            clonedThis->Parent = parentNode.get();
            yamlResouce.second.StoreString(std::string(key));
            StringView keyView = StringView(*yamlResouce.second.WriteData.back());
            NodePtr keyNode = CreateNodePtr();
            keyNode->Value = keyView;
            keyNode->LineNumber = LineNumber;
            
            mpark::get_if<OrderedMap>(&parentNode->Value)->InsertedKeys.push_back(keyNode);
            mpark::get_if<OrderedMap>(&parentNode->Value)->Map[keyNode] = clonedThis;
            mpark::get_if<OrderedMap>(&parentNode->Value)->StringMap[keyView] = clonedThis;
            return clonedThis;
        }
        
        inline DS::Result<void> Node::RemoveMapChild(StringView key)
        {
            DS_ASSERT_TRUE(IsMap());
            OrderedMap& orderedMap = *mpark::get_if<OrderedMap>(&Value);
            if(orderedMap.StringMap.count(key) == 0)
                return {};
            
            //NodePtr valueNode = orderedMap.StringMap[key];
            NodePtr keyNode = nullptr;
            int keyNodeIndex = -1;
            for(int i = 0; i < orderedMap.InsertedKeys.size(); ++i)
            {
                if( orderedMap.InsertedKeys[i]->IsScalar() && 
                    orderedMap.InsertedKeys.at(i)->GetScalar<StringView>().Value() == key)
                {
                    keyNode = orderedMap.InsertedKeys[i];
                    keyNodeIndex = i;
                    break;
                }
            }
            
            DS_ASSERT_TRUE(keyNodeIndex != -1);
            orderedMap.InsertedKeys.erase(orderedMap.InsertedKeys.begin() + keyNodeIndex);
            orderedMap.Map.erase(keyNode);
            orderedMap.StringMap.erase(key);
            return {};
        }
        
        inline DS::Result<void> Node::ToString(std::string& outString) const
        {
            std::stack<const Node*> nodeStack;
            std::stack<std::string> nodePrefixStack;
            std::stack<int> nodeIndentLevelStack;
            
            //if(!mpark::holds_alternative<OrderedMap>(Value) && !mpark::holds_alternative<Sequence>(Value))
            //    return DS_ERROR_MSG("Starting value must be a map or sequence");
            
            nodeStack.push(this);
            nodePrefixStack.push("");
            nodeIndentLevelStack.push(0);
            
            while(!nodeStack.empty())
            {
                outString += nodePrefixStack.top();
                
                /* Scalar Output Format:
                <prefix>[anchor]<scalar><newline>
                */
                if(mpark::holds_alternative<ScalarValue>(nodeStack.top()->Value))
                {
                    if(!nodeStack.top()->Anchor.empty())
                        outString += " &" + std::string(nodeStack.top()->Anchor) + " ";
                    
                    outString += ScalarToString(*mpark::get_if<ScalarValue>(&(nodeStack.top()->Value)));
                    outString += "\n";
                    nodeStack.pop();
                    nodePrefixStack.pop();
                    nodeIndentLevelStack.pop();
                }
                /* Alias Output Format:
                <prefix>*<alias><newline>
                */
                else if(mpark::holds_alternative<Alias>(nodeStack.top()->Value))
                {
                    if(!nodeStack.top()->Anchor.empty())
                        return DS_ERROR_MSG("Alias node cannot have anchor");
                    
                    outString += AliasToString(*mpark::get_if<Alias>(&(nodeStack.top()->Value)));
                    outString += "\n";
                    nodeStack.pop();
                    nodePrefixStack.pop();
                    nodeIndentLevelStack.pop();
                }
                /* Sequence Output Format:
                <prefix>-   <child>
                <indent>-   <child>
                */
                else if(mpark::holds_alternative<Sequence>(nodeStack.top()->Value))
                {
                    const Sequence* seq = mpark::get_if<Sequence>(&nodeStack.top()->Value);
                    int indentLevel = nodeIndentLevelStack.top();
                    std::string prefix = nodePrefixStack.top();
                    nodeStack.pop();
                    nodePrefixStack.pop();
                    nodeIndentLevelStack.pop();
                    if(seq->empty())
                        continue;
                    //We iterate from back to front since we are pushing to a stack
                    for(int i = seq->size() - 1; i >= 0; --i)
                    {
                        nodeStack.push((*seq)[i].get());
                        nodePrefixStack.push(std::string(indentLevel, ' ') + "-   ");
                        nodeIndentLevelStack.push(indentLevel + 4);
                    }
                    nodePrefixStack.top() = "-   ";     //No need to indent for the first one
                }
                /* Map Output Format:
                <prefix><scalar key>: <child scalar value>
                <indent><scalar key>: [child sequqence anchor]
                <indent><child sequence>
                <indent><scalar key>: [child map anchor]
                <indent + 4><child map>
                */
                else if(mpark::holds_alternative<OrderedMap>(nodeStack.top()->Value))
                {
                    const OrderedMap& map = *mpark::get_if<OrderedMap>(&(nodeStack.top()->Value));
                    int indentLevel = nodeIndentLevelStack.top();
                    nodeStack.pop();
                    nodePrefixStack.pop();
                    nodeIndentLevelStack.pop();
                    
                    if(map.InsertedKeys.empty())
                        continue;
                    
                    //We iterate from back to front since we are pushing to a stack
                    for(int i = map.InsertedKeys.size() - 1; i >= 0; --i)
                    {
                        std::string prefix;
                        if(mpark::holds_alternative<ScalarValue>(map.InsertedKeys[i]->Value))
                            prefix = (std::string)*mpark::get_if<ScalarValue>(&map.InsertedKeys[i]->Value);
                        else if(mpark::holds_alternative<Alias>(map.InsertedKeys[i]->Value))
                        {
                            //TODO: Add support for alias map key
                            const Alias& aliasVal = *mpark::get_if<Alias>(&map.InsertedKeys[i]->Value);
                            if(aliasVal.Value != "<<")
                            {
                                return DS_ERROR_MSG(DS_STR("Merge key must be <<, \"") + 
                                                    DS_STR(aliasVal.Value) + "\" is found instead");
                            }
                            else
                                prefix = (std::string)aliasVal.Value;
                        }
                        else
                            return DS_ERROR_MSG("Complex key is not supported");
                        
                        prefix += ": ";

                        //We don't add indent for the first key/value
                        if(i != 0)
                            prefix = std::string(indentLevel, ' ') + prefix;
                        
                        NodePtr childNode = map.Map.at(map.InsertedKeys[i]);
                        std::string anchor =    childNode->Anchor.empty() ? 
                                                "" : 
                                                "&" + std::string(childNode->Anchor);
                        if(mpark::holds_alternative<Sequence>(childNode->Value))
                        {
                            nodeStack.push(childNode.get());
                            nodePrefixStack.push(   prefix + anchor + "\n" + 
                                                    std::string(indentLevel, ' '));
                            nodeIndentLevelStack.push(indentLevel);
                        }
                        else if(mpark::holds_alternative<OrderedMap>(childNode->Value))
                        {
                            nodeStack.push(childNode.get());
                            nodePrefixStack.push(   prefix + anchor + "\n" + 
                                                    std::string(indentLevel + 4, ' '));
                            nodeIndentLevelStack.push(indentLevel + 4);
                        }
                        else if(mpark::holds_alternative<ScalarValue>(childNode->Value) ||
                                mpark::holds_alternative<Alias>(childNode->Value))
                        {
                            nodeStack.push(childNode.get());
                            nodePrefixStack.push(prefix);
                            nodeIndentLevelStack.push(indentLevel);
                        }
                        else
                            return DS_ERROR_MSG("Invalid type");
                    } //for(int i = map.InsertedKeys.size() - 1; i >= 0; --i)
                } //else if(mpark::holds_alternative<OrderedMap>(nodeStack.top()->Value))
                else
                    return DS_ERROR_MSG("Invalid type");
            } //while(!nodeStack.empty())
            
            return {};
        }
        
        inline DS::Result<NodePtr> Node::Clone(bool shallow, ResourceHandle& yamlResouce) const
        {
            std::stack<std::pair<NodePtr, const Node*>> nodesToCloneStack;    //Dst, src
            
            {
                NodePtr clonedNode = CreateNodePtr();
                clonedNode->Parent = nullptr;
                nodesToCloneStack.push(std::make_pair(clonedNode, this));
            }
            
            NodePtr returnNode = nodesToCloneStack.top().first;
            
            while(!nodesToCloneStack.empty())
            {
                NodePtr dst = nodesToCloneStack.top().first;
                const Node* src = nodesToCloneStack.top().second;
                nodesToCloneStack.pop();
                
                CopyVariant(dst->Value, src->Value).DS_TRY();
                dst->Anchor = src->Anchor;
                dst->LineNumber = src->LineNumber;
                
                if(shallow)
                    break;
                
                yamlResouce.second.StoreString(std::string(src->Anchor));
                dst->Anchor = StringView(*yamlResouce.second.WriteData.back());
                
                switch(src->GetType())
                {
                    case NodeType::Scalar:
                    {
                        std::string val = src->GetScalar<std::string>().DS_TRY();
                        yamlResouce.second.StoreString(val);
                        dst->Value = ScalarValue(*yamlResouce.second.WriteData.back());
                        break;
                    }
                    case NodeType::Alias:
                    {
                        std::string val = src->GetAlias<std::string>().DS_TRY();
                        yamlResouce.second.StoreString(val);
                        dst->Value = Alias{ StringView(*yamlResouce.second.WriteData.back()) };
                        break;
                    }
                    case NodeType::Sequence:
                    {
                        DS_ASSERT_TRUE(dst->IsSequence());
                        mpark::get_if<Sequence>(&dst->Value)->clear();
                        for(int i = 0; i < src->GetChildrenCount(); ++i)
                        {
                            NodePtr clonedNode = CreateNodePtr();
                            clonedNode->Parent = dst.get();
                            nodesToCloneStack.push(std::make_pair(  clonedNode, 
                                                                    src->GetSequenceChildNode(i).get()));
                            mpark::get_if<Sequence>(&dst->Value)->push_back(clonedNode);
                        }
                        break;
                    }
                    case NodeType::Map:
                    {
                        DS_ASSERT_TRUE(dst->IsMap());
                        OrderedMap& dstMap = *mpark::get_if<OrderedMap>(&dst->Value);
                        
                        dstMap.InsertedKeys.clear();
                        dstMap.Map.clear();
                        dstMap.StringMap.clear();
                        
                        for(int i = 0; i < src->GetChildrenCount(); ++i)
                        {
                            NodePtr clonedKeyNode = CreateNodePtr();
                            NodePtr clonedValNode = CreateNodePtr();
                            clonedKeyNode->Parent = dst.get();
                            clonedValNode->Parent = dst.get();
                            
                            ConstNodePtr keyNode = src->GetMapKeyNodeAt(i);
                            DS_ASSERT_TRUE(keyNode->IsScalar() || keyNode->IsAlias());
                            ConstNodePtr valueNode =  src->GetMapValueNodeAt(i);
                            
                            nodesToCloneStack.push(std::make_pair(clonedKeyNode, keyNode.get()));
                            nodesToCloneStack.push(std::make_pair(clonedValNode, valueNode.get()));
                            
                            dstMap.InsertedKeys.push_back(clonedKeyNode);
                            dstMap.Map[clonedKeyNode] = clonedValNode;
                            
                            if(keyNode->IsScalar())
                            {
                                std::string key = keyNode->GetScalar<std::string>().DS_TRY();
                                yamlResouce.second.StoreString(key);
                                StringView keyView = StringView(*yamlResouce.second.WriteData.back());
                                dstMap.StringMap[keyView] = clonedValNode;
                            }
                        }
                        break;
                    }
                    default:
                        return DS_ERROR_MSG("Invalid node type: " + DS_STR((int)src->GetType()));
                }
            } //while(!nodesToCloneStack.empty())

            return returnNode;
        }
    }
}

namespace
{
    std::string EscapeString(const runcpp2::StringView string)
    {
        std::string outString = "";
        for(int i = 0; i < string.size(); ++i)
        {
            switch(string[i])
            {
                case '\\':
                    outString += "\\\\";
                    break;
                case '\0':
                    outString += "\\0";
                    break;
                case '\b':
                    outString += "\\b";
                    break;
                case '\n':
                    outString += "\\n";
                    break;
                case '\r':
                    outString += "\\r";
                    break;
                case '\t':
                    outString += "\\t";
                    break;
                default:
                    outString += string[i];
                    break;
            }
        }
        return outString;
    }
    
    std::string ScalarToString(const runcpp2::YAML::ScalarValue& scalar)
    {
        return std::string("\"") + EscapeString(scalar) + "\"";
    }
    
    std::string AliasToString(const runcpp2::YAML::Alias& alias)
    {
        return std::string("*") + std::string(alias.Value.data(), alias.Value.size());
    }
    
    template<bool pushToStack, typename YamlValueType>
    DS::Result<void> AddValueToMap( std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                            int>>& nodeValCountStack,
                                    const YamlValueType& newVal,
                                    const yaml_event_t& event)
    {
        nodeValCountStack.top().second = 0;
        DS_ASSERT_TRUE(nodeValCountStack.top().first->IsMap());
        runcpp2::YAML::OrderedMap* currentMap = 
            mpark::get_if<runcpp2::YAML::OrderedMap>(&nodeValCountStack.top().first->Value);
        DS_ASSERT_NOT_EQ(currentMap, nullptr);
        DS_ASSERT_FALSE(currentMap->InsertedKeys.empty());
        const runcpp2::YAML::NodePtr& lastKey = currentMap->InsertedKeys.back();
        
        DS_ASSERT_GT(currentMap->Map.count(lastKey), 0);
        DS_ASSERT_TRUE(currentMap->Map[lastKey] == nullptr);
        currentMap->Map[lastKey] = std::make_shared<runcpp2::YAML::Node>();
        currentMap->Map[lastKey]->Value = newVal;
        currentMap->Map[lastKey]->LineNumber = event.start_mark.line + 1;
        currentMap->Map[lastKey]->Parent = nodeValCountStack.top().first;
        
        //Add to KeyMap if key is scalar
        if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(lastKey->Value))
        {
            auto* scalarVal = mpark::get_if<runcpp2::YAML::ScalarValue>(&lastKey->Value);
            currentMap->StringMap[*scalarVal] = currentMap->Map[lastKey];
        }
        
        if(pushToStack)
        {
            nodeValCountStack.push(std::pair<   runcpp2::YAML::Node*, 
                                                int>(currentMap->Map[lastKey].get(), 0));
        }
        return {};
    }
    
    DS::Result<void> ParseHandleMap(yaml_event_t& event, 
                                    std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                            int>>& nodeValCountStack)
    {
        //ssLOG_FUNC_DEBUG();
        switch(nodeValCountStack.top().second)
        {
            //Starting node
            case -1:
                nodeValCountStack.top().first->Value = runcpp2::YAML::OrderedMap();
                ++(nodeValCountStack.top().second);
                break;
            
            //Fill key: 
            case 0:
            {
                runcpp2::YAML::NodeValue& currentParentValue = nodeValCountStack.top().first->Value;

                //We can only start a map as key in sequence node, error out
                if(!mpark::holds_alternative<runcpp2::YAML::Sequence>(currentParentValue))
                {
                    //If we are in a map node, that means it is a complex key. 
                    //We don't support complex key.
                    if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(currentParentValue))
                        return DS_ERROR_MSG("Complex key is not supported");
                    //If we are in scalar node, what?
                    else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(currentParentValue))
                    {
                        return DS_ERROR_MSG("Trying to create map in scalar node. "
                                            "Missed node creation?");
                    }
                    //If we are in alias, what?
                    else if(mpark::holds_alternative<runcpp2::YAML::Alias>(currentParentValue))
                    {
                        return DS_ERROR_MSG("Trying to create map in alias node. "
                                            "Missed node creation?");
                    }
                    //Invalid type
                    else
                    {
                        return DS_ERROR_MSG("Invalid node value type: " + 
                                            DS_STR(currentParentValue.index()));
                    }
                }
                
                runcpp2::YAML::NodePtr newNode = std::make_shared<runcpp2::YAML::Node>();
                newNode->Value = runcpp2::YAML::OrderedMap();
                newNode->LineNumber = event.start_mark.line + 1;
                newNode->Parent = nodeValCountStack.top().first;
                mpark::get_if<runcpp2::YAML::Sequence>(&currentParentValue)->push_back(newNode);
                nodeValCountStack.push(std::pair<runcpp2::YAML::Node*, int>(newNode.get(), 0));
                break;
            }
            
            //Key is filled, value is map
            case 1:
            {
                runcpp2::YAML::NodeValue& currentParentValue = nodeValCountStack.top().first->Value;
                
                //We can only fill a value after key in a map node, error out
                if(!mpark::holds_alternative<runcpp2::YAML::OrderedMap>(currentParentValue))
                {
                    if(mpark::holds_alternative<runcpp2::YAML::Sequence>(currentParentValue))
                    {
                        return DS_ERROR_MSG("Trying to fill value in sequnce node. "
                                            "Missed node creation?");
                    }
                    else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(currentParentValue))
                    {
                        return DS_ERROR_MSG("Trying to fill value in scalar node. "
                                            "Missed node creation?");
                    }
                    else if(mpark::holds_alternative<runcpp2::YAML::Alias>(currentParentValue))
                    {
                        return DS_ERROR_MSG("Trying to fill value in alias node. "
                                            "Missed node creation?");
                    }
                    //Invalid type
                    else
                    {
                        return DS_ERROR_MSG("Invalid node value type: " + 
                                            DS_STR(currentParentValue.index()));
                    }
                }
                runcpp2::YAML::OrderedMap newMap = runcpp2::YAML::OrderedMap();
                DS_UNWRAP_VOID(AddValueToMap<true>(nodeValCountStack, newMap, event));
                break;
            } //case 1:
            
            //What?
            default:
                return DS_ERROR_MSG("Invalid node counter: " + DS_STR(nodeValCountStack.top().second));
        } //switch(nodeValCountStack.top().second)
        
        if(event.data.mapping_start.anchor)
        {
            nodeValCountStack.top().first->Anchor = 
                runcpp2::StringView((const char*)event.data.mapping_start.anchor);
        }
        
        ssLOG_DEBUG("+MAP {}" << 
                    (
                        event.data.mapping_start.anchor ? 
                        std::string(" &") + std::string((char*)event.data.mapping_start.anchor) : 
                        std::string("")
                    ) <<
                    (
                        event.data.mapping_start.tag ?
                        std::string(" <") + std::string((char*)event.data.mapping_start.tag) + ">" :
                        std::string("")
                    ));

        return {};
    }
    
    DS::Result<void> ParseHandleSequence(   yaml_event_t& event, 
                                            std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                                    int>>& nodeValCountStack)
    {
        //ssLOG_FUNC_DEBUG();
        switch(nodeValCountStack.top().second)
        {
            //Starting node
            case -1:
                nodeValCountStack.top().first->Value = runcpp2::YAML::Sequence();
                ++(nodeValCountStack.top().second);
                break;

            //Fill key: 
            case 0:
            {
                runcpp2::YAML::NodeValue& currentParentValue = nodeValCountStack.top().first->Value;
                //If we are in a sequence node, this means it is a nested sequence.
                //    fill it and append to stack
                if(mpark::holds_alternative<runcpp2::YAML::Sequence>(currentParentValue))
                {
                    runcpp2::YAML::NodePtr newNode = runcpp2::YAML::CreateNodePtr();
                    newNode->Value = runcpp2::YAML::Sequence();
                    newNode->LineNumber = event.start_mark.line + 1;
                    newNode->Parent = nodeValCountStack.top().first;
                    mpark::get_if<runcpp2::YAML::Sequence>(&currentParentValue)->push_back(newNode);
                    nodeValCountStack.push(std::pair<runcpp2::YAML::Node*, int>(newNode.get(), 0));
                }
                //If we are in a map node, that means it is a complex key. 
                //We don't support complex key.
                else if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(currentParentValue))
                    return DS_ERROR_MSG("Complex key is not supported");
                //If we are in scalar node, what?
                else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(currentParentValue))
                    return DS_ERROR_MSG("Trying to create map in scalar node. Missed node creation?");
                //If we are in alias node, what?
                else if(mpark::holds_alternative<runcpp2::YAML::Alias>(currentParentValue))
                    return DS_ERROR_MSG("Trying to create map in alias node. Missed node creation?");
                //Invalid type
                else
                    return DS_ERROR_MSG("Invalid node value type: " + DS_STR(currentParentValue.index()));
                break;
            }
            
            //Key is filled, value is sequence
            case 1:
            {
                runcpp2::YAML::NodeValue& currentParentValue = nodeValCountStack.top().first->Value;
                runcpp2::YAML::Sequence newSeq = runcpp2::YAML::Sequence();
                
                //If we are in a map node, that means the value is a sequence
                //     fill it and append to stack
                if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(currentParentValue))
                    DS_UNWRAP_VOID(AddValueToMap<true>(nodeValCountStack, newSeq, event));
                else if(mpark::holds_alternative<runcpp2::YAML::Sequence>(currentParentValue))
                    return DS_ERROR_MSG("Trying to fill value in sequnce node. Missed node creation?");
                else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(currentParentValue))
                    return DS_ERROR_MSG("Trying to fill value in scalar node. Missed node creation?");
                else if(mpark::holds_alternative<runcpp2::YAML::Alias>(currentParentValue))
                    return DS_ERROR_MSG("Trying to fill value in alias node. Missed node creation?");
                //Invalid type
                else
                    return DS_ERROR_MSG("Invalid node value type: " + DS_STR(currentParentValue.index()));
                break;
            }
            
            //What?
            default:
                return DS_ERROR_MSG("Invalid node counter: " + DS_STR(nodeValCountStack.top().second));
        } //switch(nodeValCountStack.top().second)
        
        if(event.data.sequence_start.anchor)
        {
            nodeValCountStack.top().first->Anchor = 
                runcpp2::StringView((const char*)event.data.sequence_start.anchor);
        }
        ssLOG_DEBUG("+SEQ []" <<
                    (
                        event.data.sequence_start.anchor ?
                        std::string(" &") + std::string((char*)event.data.sequence_start.anchor) :
                        std::string("")
                    ) << 
                    (
                        event.data.sequence_start.tag ?
                        std::string(" <") + std::string((char*)event.data.sequence_start.tag) + ">" :
                        std::string("")
                    ));
        
        return {};
    }
    
    DS::Result<void> ParseHandleScalar( yaml_event_t& event, 
                                        std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                                int>>& nodeValCountStack)
    {
        //ssLOG_FUNC_DEBUG();
        std::string scalarStartStyle = "";
        std::string scalarEndStyle = "";
        
        switch(event.data.scalar.style) 
        {
            case YAML_PLAIN_SCALAR_STYLE:
                scalarStartStyle = " :";
                break;
            case YAML_SINGLE_QUOTED_SCALAR_STYLE:
                scalarStartStyle = " '";
                scalarEndStyle = "'";
                break;
            case YAML_DOUBLE_QUOTED_SCALAR_STYLE:
                scalarStartStyle = " \"";
                scalarEndStyle = "\"";
                break;
            case YAML_LITERAL_SCALAR_STYLE:
                scalarStartStyle = " |";
                break;
            case YAML_FOLDED_SCALAR_STYLE:
                scalarStartStyle = " >";
                break;
            case YAML_ANY_SCALAR_STYLE:
                ssLOG_ERROR("Unrecognized scalar style");
                break;
        }
        
        ssLOG_DEBUG("=VAL" <<
                    (
                        event.data.scalar.anchor ?
                        std::string(" &") + std::string((char*)event.data.scalar.anchor) :
                        std::string("")
                    ) << 
                    (
                        event.data.scalar.tag ?
                        std::string(" <") + std::string((char*)event.data.scalar.tag) + ">" :
                        std::string("")
                    ) << 
                    scalarStartStyle << 
                    EscapeString(runcpp2::StringView(   (char*)event.data.scalar.value, 
                                                        event.data.scalar.length)) <<
                    scalarEndStyle);
        
        runcpp2::StringView scalarView = runcpp2::StringView(   (const char*)event.data.scalar.value, 
                                                                event.data.scalar.length);
        switch(nodeValCountStack.top().second)
        {
            //Starting node
            case -1:
                return DS_ERROR_MSG("Starting node must be map or sequence");
            
            //Fill key: 
            case 0:
            {
                if(event.data.scalar.anchor)
                {
                    return DS_ERROR_MSG("Parsing error where key " + (std::string)scalarView + 
                                        " has anchor " + (char*)event.data.scalar.anchor);
                }
                
                runcpp2::YAML::NodeValue& nodeValue = nodeValCountStack.top().first->Value;
                runcpp2::YAML::NodePtr newNode = runcpp2::YAML::CreateNodePtr();
                newNode->LineNumber = event.start_mark.line + 1;
                newNode->Parent = nodeValCountStack.top().first;
                bool mergeKey = scalarView == "<<" && 
                                event.data.scalar.style == YAML_PLAIN_SCALAR_STYLE &&
                                mpark::holds_alternative<runcpp2::YAML::OrderedMap>(nodeValue);
                if(!mergeKey)
                    newNode->Value = scalarView;
                else
                    newNode->Value = runcpp2::YAML::Alias{scalarView};
                
                //If we are in a sequence node, we just need to append the value
                if(mpark::holds_alternative<runcpp2::YAML::Sequence>(nodeValue))
                    mpark::get_if<runcpp2::YAML::Sequence>(&nodeValue)->emplace_back(newNode);
                //If we are in a map node, we need to set the key
                else if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(nodeValue))
                {
                    mpark::get_if<runcpp2::YAML::OrderedMap>(&nodeValue)->InsertedKeys
                                                                        .push_back(newNode);
                    mpark::get_if<runcpp2::YAML::OrderedMap>(&nodeValue)->Map[newNode] = nullptr;
                    ++(nodeValCountStack.top().second);
                }
                //We should never in a scalar node
                else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(nodeValue))
                    return DS_ERROR_MSG("Should not be in a scalar node");
                //We should never in a alias node
                else if(mpark::holds_alternative<runcpp2::YAML::Alias>(nodeValue))
                    return DS_ERROR_MSG("Should not be in a alias node");
                //Invalid type
                else
                    return DS_ERROR_MSG("Invalid node value type: " + DS_STR(nodeValue.index()));
                break;
            }
            
            //Key is filled, value is scalar
            case 1:
            {
                runcpp2::YAML::NodeValue& nodeValue = nodeValCountStack.top().first->Value;
                
                //If we are in a map node, that means the value is scalar
                if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(nodeValue))
                    DS_UNWRAP_VOID(AddValueToMap<false>(nodeValCountStack, scalarView, event));
                else if(mpark::holds_alternative<runcpp2::YAML::Sequence>(nodeValue))
                    return DS_ERROR_MSG("Trying to fill value in sequnce node. Missed node creation?");
                //We should never in a scalar node
                else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(nodeValue))
                    return DS_ERROR_MSG("Should not be in a scalar node");
                //We should never in a alias node
                else if(mpark::holds_alternative<runcpp2::YAML::Alias>(nodeValue))
                    return DS_ERROR_MSG("Should not be in a alias node");
                //Invalid type
                else
                    return DS_ERROR_MSG("Invalid node value type: " + DS_STR(nodeValue.index()));
            
                if(event.data.scalar.anchor)
                {
                    auto& mapVal = *mpark::get_if<runcpp2::YAML::OrderedMap>(&nodeValue);
                    mapVal.Map[mapVal.InsertedKeys.back()]->Anchor = 
                        runcpp2::StringView((const char*)event.data.scalar.anchor);
                }
                break;
            }
            
            //What?
            default:
                return DS_ERROR_MSG("Invalid node counter: " + DS_STR(nodeValCountStack.top().second));
        }
        
        return {};
    }
    
    DS::Result<void> ParseHandleAlias(  yaml_event_t& event, 
                                        std::stack<std::pair<   runcpp2::YAML::Node*, 
                                                                int>>& nodeValCountStack)
    {
        //ssLOG_FUNC_DEBUG();
        ssLOG_DEBUG("=ALI *" << (char*)event.data.alias.anchor);
        
        switch(nodeValCountStack.top().second)
        {
            //Starting node
            case -1:
                return DS_ERROR_MSG("Starting node must be map or sequence");

            //Fill key: 
            case 0:
            {
                runcpp2::YAML::NodeValue& currentParentValue = nodeValCountStack.top().first->Value;
                runcpp2::YAML::NodePtr newNode = runcpp2::YAML::CreateNodePtr();
                newNode->Value = 
                    runcpp2::YAML::Alias { runcpp2::StringView((const char*)event.data.alias.anchor) };
                
                newNode->LineNumber = event.start_mark.line + 1;
                newNode->Parent = nodeValCountStack.top().first;
                
                //If we are in a sequence node, we just need to append the alias
                if(mpark::holds_alternative<runcpp2::YAML::Sequence>(currentParentValue))
                    mpark::get_if<runcpp2::YAML::Sequence>(&currentParentValue)->emplace_back(newNode);
                //Alias should never be map key
                //TODO: Actually, it can. But I guess we can forget about it for now?
                else if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(currentParentValue))
                    return DS_ERROR_MSG("Alias cannot be map key");
                //We should never in a scalar node
                else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(currentParentValue))
                    return DS_ERROR_MSG("Should not be in a scalar node");
                //We should never in a alias node
                else if(mpark::holds_alternative<runcpp2::YAML::Alias>(currentParentValue))
                    return DS_ERROR_MSG("Should not be in a alias node");
                //Invalid type
                else
                    return DS_ERROR_MSG("Invalid node value type: " + DS_STR(currentParentValue.index()));
                break;
            }
            
            //Key is filled, value is alias
            case 1:
            {
                runcpp2::YAML::NodeValue& currentParentValue = nodeValCountStack.top().first->Value;
                
                //If we are in a map node, that means the value is alias
                if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(currentParentValue))
                {
                    runcpp2::YAML::Alias alias = 
                        { runcpp2::StringView((const char*)event.data.alias.anchor) };
                    
                    DS_UNWRAP_VOID(AddValueToMap<false>(nodeValCountStack, alias, event));
                }
                else if(mpark::holds_alternative<runcpp2::YAML::Sequence>(currentParentValue))
                    return DS_ERROR_MSG("Trying to fill value in sequnce node. Missed node creation?");
                //We should never in a scalar node
                else if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(currentParentValue))
                    return DS_ERROR_MSG("Should not be in a scalar node");
                //We should never in a alias node
                else if(mpark::holds_alternative<runcpp2::YAML::Alias>(currentParentValue))
                    return DS_ERROR_MSG("Should not be in a alias node");
                //Invalid type
                else
                    return DS_ERROR_MSG("Invalid node value type: " + DS_STR(currentParentValue.index()));
                break;
            }
            
            //What?
            default:
                return DS_ERROR_MSG("Invalid node counter: " + DS_STR(nodeValCountStack.top().second));
        } //switch(nodeValCountStack.top().second)
        
        return {};
    }
    
    DS::Result<void> CopyVariant(runcpp2::YAML::NodeValue& dest, const runcpp2::YAML::NodeValue& src)
    {
        if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(src))
            dest = *mpark::get_if<runcpp2::YAML::ScalarValue>(&src);
        else if(mpark::holds_alternative<runcpp2::YAML::Alias>(src))
            dest = *mpark::get_if<runcpp2::YAML::Alias>(&src);
        else if(mpark::holds_alternative<runcpp2::YAML::Sequence>(src))
            dest = *mpark::get_if<runcpp2::YAML::Sequence>(&src);
        else if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(src))
            dest = *mpark::get_if<runcpp2::YAML::OrderedMap>(&src);
        else
            return DS_ERROR_MSG("Invalid type");
        
        return {};
    }
    
    DS::Result<void> UpdateParentsRecursively(runcpp2::YAML::Node& currentNode)
    {
        //NOTE: Parent of currentNode is correct, but not its children
        std::stack<runcpp2::YAML::Node*> nodesToProcess;
        nodesToProcess.push(&currentNode);
        
        while(!nodesToProcess.empty())
        {
            runcpp2::YAML::Node& node = *nodesToProcess.top();
            nodesToProcess.pop();
            
            if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(node.Value))
                continue;
            else if(mpark::holds_alternative<runcpp2::YAML::Alias>(node.Value))
                continue;
            else if(mpark::holds_alternative<runcpp2::YAML::Sequence>(node.Value))
            {
                auto& seq = *mpark::get_if<runcpp2::YAML::Sequence>(&node.Value);
                for(int i = 0; i < seq.size(); ++i)
                {
                    if(seq[i]->Parent != &node)
                    {
                        runcpp2::YAML::NodePtr newNode = runcpp2::YAML::CreateNodePtr();
                        newNode->Anchor = seq[i]->Anchor;
                        newNode->LineNumber = seq[i]->LineNumber;
                        newNode->Parent = &node;
                        DS_UNWRAP_VOID(CopyVariant(newNode->Value, seq[i]->Value));
                        seq[i] = newNode;
                    }
                    
                    nodesToProcess.push(seq[i].get());
                }
            }
            else if(mpark::holds_alternative<runcpp2::YAML::OrderedMap>(node.Value))
            {
                auto& map = *mpark::get_if<runcpp2::YAML::OrderedMap>(&node.Value);
                for(int i = 0; i < map.InsertedKeys.size(); ++i)
                {
                    runcpp2::YAML::NodePtr currentKey = map.InsertedKeys[i];
                    runcpp2::YAML::NodePtr currentVal = map.Map[currentKey];
                    
                    if(currentKey->Parent != &node || currentVal->Parent != &node)
                    {
                        runcpp2::YAML::NodePtr newKey = runcpp2::YAML::CreateNodePtr();
                        newKey->Anchor = currentKey->Anchor;
                        newKey->LineNumber = currentKey->LineNumber;
                        newKey->Parent = &node;
                        DS_UNWRAP_VOID(CopyVariant(newKey->Value, currentKey->Value));
                        
                        runcpp2::YAML::NodePtr newVal = runcpp2::YAML::CreateNodePtr();
                        newVal->Anchor = currentKey->Anchor;
                        newVal->LineNumber = currentKey->LineNumber;
                        newVal->Parent = &node;
                        DS_UNWRAP_VOID(CopyVariant(newVal->Value, currentVal->Value));
                        
                        //Replace entries with new key and values
                        {
                            map.InsertedKeys[i] = newKey;
                            
                            map.Map.erase(currentKey);
                            map.Map[newKey] = newVal;
                            
                            if(mpark::holds_alternative<runcpp2::YAML::ScalarValue>(currentKey->Value))
                            {
                                auto& keyScalar = 
                                    *mpark::get_if<runcpp2::YAML::ScalarValue>(&currentKey->Value);
                                
                                map.StringMap[keyScalar] = newVal;
                            }
                        }
                        
                        nodesToProcess.push(newKey.get());
                        nodesToProcess.push(newVal.get());
                    }
                    else
                    {
                        nodesToProcess.push(currentKey.get());
                        nodesToProcess.push(currentVal.get());
                    }
                }
            }
            else
                return DS_ERROR_MSG("Invalid type");
        }
        
        return {};
    }
    
    //NOTE: This doesn't update OrderedMap.StringMap
    DS::Result<void> ResolveMergeKey(   runcpp2::YAML::OrderedMap& currentMap, 
                                        int mergeKeyIndex,
                                        const std::unordered_map<   runcpp2::StringView, 
                                                                    runcpp2::YAML::Node*>& anchors)
    {
        //ssLOG_FUNC_DEBUG();
        runcpp2::YAML::NodePtr currentKey = currentMap.InsertedKeys[mergeKeyIndex];
        
        //We trust that the key at `mergeKeyIndex` is a merge key
        runcpp2::YAML::NodePtr currentValueNode = currentMap.Map[currentKey];
        std::string defaultErrorMessage = "We are expecting either an alias or sequence of alias";
        
        std::vector<runcpp2::YAML::NodePtr> aliasNodes;
        std::vector<runcpp2::YAML::OrderedMap*> mapsForMerging;
        
        if(mpark::holds_alternative<runcpp2::YAML::Alias>(currentValueNode->Value))
            aliasNodes.push_back(currentValueNode);
        else if(mpark::holds_alternative<runcpp2::YAML::Sequence>(currentValueNode->Value))
        {
            auto& sequence = *mpark::get_if<runcpp2::YAML::Sequence>(&currentValueNode->Value);
            for(int i = 0; i < sequence.size(); ++i)
                aliasNodes.push_back(sequence[i]);
        }
        else
            return DS_ERROR_MSG(defaultErrorMessage);
        
        for(int i = 0; i < aliasNodes.size(); ++i)
        {
            runcpp2::YAML::NodeValue& currentAliasValue = aliasNodes[i]->Value;
            
            if(!mpark::holds_alternative<runcpp2::YAML::Alias>(currentAliasValue))
                return DS_ERROR_MSG(defaultErrorMessage);
            
            auto& alias = *mpark::get_if<runcpp2::YAML::Alias>(&currentAliasValue);
            if(anchors.count(alias.Value) == 0)
            {
                return DS_ERROR_MSG("Cannot find anchor value: " + DS_STR(alias.Value) + 
                                    " on line " + DS_STR(aliasNodes[i]->LineNumber));
            }
            
            runcpp2::YAML::Node* anchorNode = anchors.at(alias.Value);
            if(!mpark::holds_alternative<runcpp2::YAML::OrderedMap>(anchorNode->Value))
            {
                return DS_ERROR_MSG("Target anchor must be a map. Line " + 
                                    DS_STR(aliasNodes[i]->LineNumber));
            }
            
            mapsForMerging.emplace_back(mpark::get_if<runcpp2::YAML::OrderedMap>(&anchorNode->Value));
        }
        
        //For each map that needs to be merged, iterating their children and add it to our map
        for(int i = 0; i < mapsForMerging.size(); ++i)
        {
            for(int j = mapsForMerging[i]->InsertedKeys.size() - 1; j >= 0; --j)
            {
                runcpp2::YAML::NodePtr keyToInsert = mapsForMerging[i]->InsertedKeys[j];
                runcpp2::YAML::NodePtr valueToInsert = mapsForMerging[i]->Map[keyToInsert];
                
                runcpp2::YAML::ScalarValue keyView = 
                    keyToInsert->GetScalar<runcpp2::YAML::ScalarValue>().DefaultOr();
                
                //If duplicate key exist, that's override key, don't merge it
                if(!keyView.empty() && currentMap.StringMap.count(keyView) > 0)
                    continue;
                
                //Add key and value
                currentMap.InsertedKeys.insert( currentMap.InsertedKeys.begin() + mergeKeyIndex + 1, 
                                                keyToInsert);
                currentMap.Map[keyToInsert] = valueToInsert;
            }
        }
        
        //Finally remove the merge key entry
        currentMap.Map.erase(currentKey);
        currentMap.InsertedKeys.erase(currentMap.InsertedKeys.begin() + mergeKeyIndex);
        
        return {};
    }
} //namespace


#endif
