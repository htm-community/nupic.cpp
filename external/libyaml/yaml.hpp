






#   NOTE: not finished.
#   REFERENCES:
# https://gist.github.com/meffie/89d106a86b81c579c2b2a1895ffa18b0
# https://github.com/yaml/libyaml
# https://www.wpsoftware.net/andrew/pages/libyaml.html
# https://pyyaml.org/wiki/LibYAML
# https://github.com/jbeder/yaml-cpp/wiki/Tutorial   for yaml-cpp
#pragma once

#include <stdexcept>


namespace YAML {

enum NodeType
{
  Null = 0,
  Scalar,
  Sequence,
  Map
};

/****************************************************************************/

class YamlError : public std::runtime_error
{
public:
  YamlError(const std::string & what, size_t line, size_t col,
             const std::string & context = "", size_t ctx_line = -1)
      : std::runtime_error(genErrMsg(what, line, col, context, ctx_line))
  {}

private:
  std::string static genErrMsg(const std::string & what, size_t line, size_t col,
                               const std::string & context, size_t ctx_line) {
    std::ostringstream error;
    error << what
          << " line " << line + 1 << " column " << col + 1
          << " " << context;
    if (ctx_line != -1) {
        error << " from line " << ctx_line + 1;
    }
    return error.str();
  }
};

/****************************************************************************/


class Node : public std::vector<Node> {
public:


  Node() { type_ = Null; }
  ~Node() { }
  //Node(const Node& rhs);  ??
  //Node(Node&& rhs);  ??
  explicit Node(NodeType type) { type_ = type;}

  NodeType::value Type() const { return type_; }
  //bool IsDefined() const;
  bool IsNull() const { return Type() == NodeType::Null; }
  bool IsScalar() const { return Type() == NodeType::Scalar; }
  bool IsSequence() const { return Type() == NodeType::Sequence; }
  bool IsMap() const { return Type() == NodeType::Map; }

  // bool conversions
  YAML_CPP_OPERATOR_BOOL()
  bool operator!() const { return !IsDefined(); }

  // access
  template <typename T>
  T as() const;
   char as<char>() { return (char)stoi(value_); }
   short as<short>() { return (short)stoi(value_); }
   unsigned short as<unsigned short>() { return (unsigned short)stoul(value_); }
   int as<int>() { return stoul(value_); }
   unsigned int as<uint>() { return stoul(value_); }
   long long as<long long>() { return stoll(value_); }
   unsigned long long as<unsigned long long>() { return stoull(value_); }
   float as<float>() { return stof(value_); }
   double as<double>() { return stod(value_); }

  template <typename T, typename S>
  T as(const S& fallback) const { if (value_.empty()) return (T)fallback; return as<T>();}


  const std::string& Tag() const { return tag_; }


  // assignment
  //bool is(const Node& rhs) const;
  //template <typename T>
  //Node& operator=(const T& rhs);
  //Node& operator=(const Node& rhs);


  // Set Node to undefined
  void clear() override { tag_ = ""; value_ = ""; type_ = Null; defined_=false; std::vector::clear(); }

  // indexing
  //template <typename Key>
  //const Node operator[](const Key& key) const;

  //template <typename Key>
  //Node operator[](const Key& key);

  //template <typename Key>
  //bool remove(const Key& key);

  //const Node operator[](const Node& key) const;
  //Node operator[](const Node& key);
  bool remove(const Node& key);

  // map
  //template <typename Key, typename Value>
  //void force_insert(const Key& key, const Value& value);

  void addScaler(const std::string& value, const std::string& tag="") {
    Node n(NodeType::Scalar);
    n.parent_ = this;
    n.tag_ = tag;
    n.value_ = value;
    parent->push_back(n);
  }
  void beginSeq() {
    Node n(NodeType::Sequence);
    parent_->push_back(n)
  }
  void endSeq() {
    parent_ = parent_->parent_;
  }
  void beginMap() {
    Node n(NodeType::Map);
    parent_->push_back(n)
  }
  void endMap() {

    parent_ = parent_->parent_;
  }

private:
  std::string tag_;
  std::string value_;
  enum NodeType type_;
  Node *parent_;

};

/****************************************************************************/
class YAML
{
public:
    static YAML::Node Load(std::string& yamlstring) {
      YAML yaml(yamlstrng);
      return yaml.root;
    }
    YAMLParser(std::string& yamlstring) {
      stringstream stream(yamlstring);
      initialize(stream);
    }
    YAMLParser(std::istream & stream) {
      initialize(stream);
    }
    ~YAMLParser()
    {
        if (m_hasEvent) { yaml_event_delete(&event); }
        yaml_parser_delete(&m_parser);
    }

private:
    void initialize(std::istream & stream) {
        m_done = false;
        m_hasEvent = false;
        if (yaml_parser_initialize(&m_parser) == 0) {
            throw std::runtime_error("YAML parser initialization failed");
        }
        yaml_parser_set_input(&m_parser, readHandler, &stream);
        parse();
    }

    static int readHandler(void * stream_ptr, unsigned char * buffer,
                           size_t size, size_t * size_read) {
        std::istream & stream = *reinterpret_cast<std::istream*>(stream_ptr);
        if (!stream.eof()) {
            stream.read(reinterpret_cast<std::istream::char_type*>(buffer), size);
            *size_read = stream.gcount();
        } else {
            *size_read = 0;
        }
        return stream.bad() ? 0 : 1;
    }

    YAML::Node parse() {
      yaml_event_t event;
      YAML::Node* node = &m_root;

      m_done = false;
      do {
        if (m_hasEvent) {
            yaml_event_delete(&event);
            m_hasEvent = false;
        }
        if (yaml_parser_parse(&m_parser, &event) == 0) {
            throw YAMLParser::ParseError(
                m_parser.problem,
                m_parser.problem_mark.line, m_parser.problem_mark.column,
                m_parser.context, m_parser.context_mark.line
            );
        }
        m_hasEvent = true;

        switch(event.type)
        {
        case YAML_NO_EVENT: puts("No event!"); break;
        /* Stream start/end */
        case YAML_STREAM_START_EVENT:  break;
        case YAML_STREAM_END_EVENT:    m_done = true;  break;
        /* Block delimeters */
        case YAML_DOCUMENT_START_EVENT: m_root.clear(); node = &m_root; break;
        case YAML_DOCUMENT_END_EVENT:                                   break;
        case YAML_SEQUENCE_START_EVENT: node = node->beginSeq();        break;
        case YAML_SEQUENCE_END_EVENT:   node = node->endSeq();          break;
        case YAML_MAPPING_START_EVENT:  node = node->beginMap();        break;
        case YAML_MAPPING_END_EVENT:    node = node->endMap()           break;
        /* Data */
        case YAML_ALIAS_EVENT:  node->setAnchor(event.data.alias.anchor); break;
        case YAML_SCALAR_EVENT: node->setScalar(event.data.scalar.value); break;  // TODO: set tag
        }
        if(event.type != YAML_STREAM_END_EVENT)
          yaml_event_delete(&event);
      } while(!m_done);
      yaml_event_delete(&event);

      stream.close();
      return m_root;
    }



    YAML::Node      m_root;
    bool            m_done;         ///< Set when end of stream was reached
    yaml_parser_t   m_parser;       ///< libYAML parser object
    bool            m_hasEvent;     ///< Set when @ref event holds an event
  };




protected:

    ParseError      makeError(const std::string & what) const
    {
	return ParseError(what, m_line, m_column);
    }
private:
    size_t          m_line;
    size_t          m_column;
};



} // namespace
