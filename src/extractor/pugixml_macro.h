#ifndef _PUGIXML_MACRO_H_
#define _PUGIXML_MACRO_H_

#include <string>
#include <sstream> 
#include <iostream> 
#include <algorithm> 

#include "pugixml.hpp"
#define NODE_EMPTY(_node_) _node_.empty()

#define NODE_SERIALIZATION_string(_node_, _var_) \
        std::string _var_;          \
        if(!NODE_EMPTY(_node_))      \
        {std::ostringstream ostr; _node_.print(ostr); _var_ = ostr.str();}

#define NODE_HAS_ATTR(_node_, _name_, _var_)        \
        bool _var_ = false;                         \
        if(!NODE_EMPTY(_node_))                     \
            if(!_node_.attribute(_name_).empty())   \
                _var_ = true;


#define NODE_ATTR_VALUE(_node_, _name_, _var_, _default_) \
            const pugi::char_t* _var_ = _default_; \
            if(!NODE_EMPTY(_node_)) {\
                pugi::xml_attribute _node_attr_ = _node_.attribute(_name_);\
                if(!_node_attr_.empty()) _var_ = _node_attr_.value(); \
            }

#define NODE_ATTR_VALUE_string(_node_, _name_, _var_, _default_) \
            std::string _var_ = _default_; \
            if(!NODE_EMPTY(_node_)) {\
                pugi::xml_attribute _node_attr_ = _node_.attribute(_name_);\
                if(!_node_attr_.empty()) _var_ = _node_attr_.value(); \
            }


#define NODE_VALUE(_node_, _var_)   \
        std::string _var_;          \
        if(!NODE_EMPTY(_node_))     \
        { _var_ = _node_.value(); }

#define NODE_CHILDREN_RICHTEXT(_node_, _var_) \
        std::string _var_;  \
        for (pugi::xml_node ic = _node_.first_child(); ic; ic = ic.next_sibling()) {    \
            if(ic.type() == pugi::node_element) {   \
                NODE_SERIALIZATION_string(ic, _tmp_str_);   \
                _var_ += _tmp_str_; \
            } else if(ic.type() == pugi::node_pcdata || ic.type() == pugi::node_cdata) \
                _var_ += ic.value();  \
        }

#define NODE_CHILD(_node_, _name_, _var_) \
        pugi::xml_node _var_ = _node_.child(_name_);

#define NODE_FIRST_CHILD(_node_, _name_, _var_) \
        pugi::xml_node _var_ ;                  \
        if(!NODE_EMPTY(_node_)) {               \
            pugi::xml_node _tmp_ = _node_.first_child(); \
            while(!NODE_EMPTY(_tmp_)) {                  \
                std::string cur_name = _tmp_.name();     \
                if(cur_name == _name_) break;            \
                _tmp_ = _tmp_.next_sibling();           \
            }                                            \
            if(!NODE_EMPTY(_tmp_)) _var_ = _tmp_;        \
        }

#define NODE_NEXT(_node_, _name_, _node_next_)  \
        pugi::xml_node _node_next_;             \
        if(!NODE_EMPTY(_node_)) {               \
            pugi::xml_node _tmp_ = _node_.next_sibling();   \
            while(!NODE_EMPTY(_tmp_)) {                     \
                std::string cur_name = _tmp_.name();        \
                if(cur_name == _name_) break;               \
                _tmp_ = _tmp_.next_sibling();               \
            }                                               \
            if(!NODE_EMPTY(_tmp_)) _node_next_ = _tmp_;     \
        }

#define NODE_NEXT_SIBLING(_node_, _node_next_)  \
        pugi::xml_node _node_next_;             \
        if(!NODE_EMPTY(_node_))                 \
            _node_next_ =  _node_.next_sibling();

#define NODE_PRE_SIBLING(_node_, _node_pre_)  \
        pugi::xml_node _node_pre_ = _node_.previous_sibling();

#define NODE_CHILD_WITH_NAME_CLASS(_node_, _name_, _class_, _var_) \
        pugi::xml_node _var_ ;                           \
        if(!NODE_EMPTY(_node_)) {                        \
            pugi::xml_node _tmp_ = _node_.first_child(); \
            while(!NODE_EMPTY(_tmp_)) {                  \
                std::string cur_name = _tmp_.name();     \
                NODE_ATTR_VALUE_string(_tmp_, "class", _class_var_, "");    \
                if(cur_name == _name_ && _class_ == _class_var_) break;     \
                _tmp_ = _tmp_.next_sibling();            \
            }                                            \
            if(!NODE_EMPTY(_tmp_)) _var_ = _tmp_;        \
        }

#
#define NODE_FOREACH(_var_, _node_set_) \
        for(int i=0; i<_node_set_.size(); i++) {\
            pugi::xml_node _var_ = _node_set_[i].node()

#define NODE_FOREACH_END() \
        }

#define NODE_SET_XPATH(_doc_, _xpath_, _var_)   \
    pugi::xpath_node_set _var_;                 \
    {pugi::xpath_query _xpath_query_(_xpath_);  \
     _var_ = _xpath_query_.evaluate_node_set(doc);}

#define AUTO_NODE_BY_XPATH(_root_, _xpath_, _var_)  \
          _var_ = _root_.select_single_node(_xpath_).node()

#define AUTO_NODE_RICHTEXT_BY_XPATH(_node_, _xpath_, _var_) \
        do {pugi::xml_node _tmp_;                           \
            AUTO_NODE_BY_XPATH(_node_, _xpath_, _tmp_);     \
            if(NODE_EMPTY(_node_)) break;                  \
            for (pugi::xml_node ic = _tmp_.first_child(); ic; ic = ic.next_sibling()) {    \
                if(ic.type() == pugi::node_element) {   \
                    NODE_SERIALIZATION_string(ic, _tmp_str_);   \
                    _var_ += _tmp_str_; \
                } else if(ic.type() == pugi::node_pcdata || ic.type() == pugi::node_cdata) \
                    _var_ += ic.value();  \
            }\
        }while(0)

#define AUTO_NODE_RICHTEXT_BY_XPATH2(_node_, _xpath_, _var_, _need_tags_v_) \
        do {pugi::xml_node _tmp_;                           \
            AUTO_NODE_BY_XPATH(_node_, _xpath_, _tmp_);     \
            if(NODE_EMPTY(_node_)) break;                  \
            for (pugi::xml_node ic = _tmp_.first_child(); ic; ic = ic.next_sibling()) {    \
                std::vector<std::string>::iterator iter = std::find(_need_tags_v_.begin(), _need_tags_v_.end(), std::string(ic.name())); \
                if(ic.type() == pugi::node_element && std::find(_need_tags_v_.begin(), _need_tags_v_.end(), ic.name()) !=_need_tags_v_.end()) {   \
                    NODE_SERIALIZATION_string(ic, _tmp_str_);   \
                    _var_ += _tmp_str_; \
                } else if(ic.type() == pugi::node_pcdata || ic.type() == pugi::node_cdata) { \
                    _var_ += ic.value();  \
                } \
            }\
        }while(0)


#define AUTO_NODE_ATTR_VALUE_string(_node_, _name_, _var_, _default_) \
            do{_var_ = _default_; \
            if(!NODE_EMPTY(_node_)) {\
                pugi::xml_attribute _node_attr_ = _node_.attribute(_name_);\
                if(!_node_attr_.empty()) _var_ = _node_attr_.value(); \
            }}while(0)


#define AUTO_NODE_ATTR_BY_XPATH(_root_, _xpath_, _var_) \
        do{ pugi::xml_attribute _attr_tmp_ = _root_.select_single_node(_xpath_).attribute();   \
          if(!_attr_tmp_.empty()) \
            _var_ = _attr_tmp_.value(); \
        }while(0)

#endif

