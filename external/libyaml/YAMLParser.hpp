#ifndef YAML_PARSER_H
#define YAML_PARSER_H

#include <stddef.h>
#include <exception>
#include <istream>
#include <string>

class YAMLParser
{
public:
    class ParseError : public std::runtime_error
    {
    public:
            ParseError(const std::string & what, size_t line, size_t col,
                       const std::string & context = "", size_t ctx_line = -1)
                : std::runtime_error(genErrMsg(what, line, col, context, ctx_line))
            {}

    private:
            std::string static genErrMsg(const std::string & what, size_t line, size_t col,
                                         const std::string & context, size_t ctx_line)
            {
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

public:
    virtual         ~YAMLParser() {}
    virtual void    parse(std::istream & stream);

protected:
    virtual void    streamStart() = 0;
    virtual void    streamEnd() = 0;
    virtual void    documentStart() = 0;
    virtual void    documentEnd() = 0;
    virtual void    sequenceStart(const std::string & tag, const std::string & anchor) = 0;
    virtual void    sequenceEnd() = 0;
    virtual void    mappingStart(const std::string & tag, const std::string & anchor) = 0;
    virtual void    mappingEnd() = 0;
    virtual void    alias(const std::string & anchor) = 0;
    virtual void    scalar(const std::string & value, const std::string & tag,
                           const std::string & anchor) = 0;

    ParseError      makeError(const std::string & what) const
    { 
	return ParseError(what, m_line, m_column); 
    }
private:
    size_t          m_line;
    size_t          m_column;
};

#endif
