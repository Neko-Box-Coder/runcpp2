#ifndef RUNCPP2_LIB_YAML_WRAPPER_HPP
#define RUNCPP2_LIB_YAML_WRAPPER_HPP

#include "DSResult/DSResult.hpp"
#include "mpark/variant.hpp"
#include "nonstd/string_view.hpp"

#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <exception>
#include <limits>
#include <cctype>
#include <cstdint>

struct yaml_event_s;
typedef yaml_event_s yaml_event_t;

struct yaml_parser_s;
typedef yaml_parser_s yaml_parser_t;

namespace runcpp2
{
    using StringView = nonstd::string_view;
    
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
    
        struct Node;
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
        DS::Result<std::vector<NodePtr>> ParseYAML(StringView yamlString, ResourceHandle& outResource);
    
        DS::Result<void> ResolveAnchors(NodePtr rootNode);
        
        DS::Result<void> FreeYAMLResource(ResourceHandle& resourceHandleToFree);
        
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
    }
}


#endif
