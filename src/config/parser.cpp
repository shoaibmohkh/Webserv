/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:35:23 by marvin            #+#    #+#             */
/*   Updated: 2025/12/10 20:35:23 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../../include/config_headers/parser.hpp"

Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens), _pos(0)
{
    std::cout << "Parser created." << std::endl;
}

Parser::~Parser()
{
    std::cout << "Parser destroyed." << std::endl;
}

void Parser::error_msg(int num)
{
    if (num == 2)
        std::cerr << "Syntax Error: Missing semicolon at line in Configfile" << _tokens[_pos].line << std::endl;
    else if (num == 3)
        std::cerr << "Syntax Error: Missing brace at line in Configfile" << _tokens[_pos].line << std::endl;
    else if (num == 4)
        std::cerr << "Syntax Error: Invalid directive at line in Configfile" << _tokens[_pos].line << std::endl;    
    else
        std::cerr << "Syntax Error at line in Configfile" << _tokens[_pos].line << std::endl;
}

int Parser::uploadEnable_and_autoindex_parse(int &_pos, LocationConfig &locConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        if (_tokens[_pos].value == "on" && _tokens[_pos - 1].value == "upload_enable")
            locConfig.uploadEnable = true;
        else if (_tokens[_pos].value == "off" && _tokens[_pos - 1].value == "upload_enable")
            locConfig.uploadEnable = false;
        else if (_tokens[_pos].value == "on" && _tokens[_pos - 1].value == "autoindex")
            locConfig.autoindex = true;
        else if (_tokens[_pos].value == "off" && _tokens[_pos - 1].value == "autoindex")
            locConfig.autoindex = false;
        else
        {
            error_msg(4);
            return 0;
        }
        _pos++;
        if (_tokens[_pos].type == SEMICOLON)
            _pos++;
        else
        {
            error_msg(2);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
}

int Parser::root_index_parse(int &_pos, ServerConfig &serverConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        if (_tokens[_pos - 1].value == "root")
            serverConfig.root = _tokens[_pos].value;
        else if (_tokens[_pos - 1].value == "index")
            serverConfig.index = _tokens[_pos].value;
        _pos++;
        if (_tokens[_pos].type == SEMICOLON)
            _pos++;
        else
        {
            error_msg(2);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
    
}

int Parser::upload_parse(int &_pos, LocationConfig &locConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        locConfig.uploadStore = _tokens[_pos].value;
        _pos++;
        if (_tokens[_pos].type == SEMICOLON)
            _pos++;
        else
        {
            error_msg(2);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
    
}

int Parser::port_and_clientMaxBodySize_parse(int &_pos, ServerConfig &serverConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        if (_tokens[_pos - 1].value == "client_max_body_size")
            serverConfig.client_Max_Body_Size = atoi(_tokens[_pos].value.c_str());
        else if (_tokens[_pos - 1].value == "listen")
            serverConfig.port = atoi(_tokens[_pos].value.c_str());
        _pos++;
        if (_tokens[_pos].type == SEMICOLON)
            _pos++;
        else
        {
            error_msg(2);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
}

int Parser::return_parse(int &_pos, LocationConfig &locConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        locConfig.returnCode = atoi(_tokens[_pos].value.c_str());
        _pos++;
        if (_tokens[_pos].type == WORD)
        {
            locConfig.returnPath = _tokens[_pos].value;
            _pos++;
            if (_tokens[_pos].type == SEMICOLON)
                _pos++;
            else
            {
                error_msg(2);
                return 0;
            }
        }
        else
        {
            error_msg(4);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
}

int Parser::cgi_extension_parse(int &_pos, LocationConfig &locConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        std::string ext = _tokens[_pos].value;
        _pos++;
        if (_tokens[_pos].type == WORD)
        {
            std::string Path = _tokens[_pos].value;
            locConfig.cgiExtensions[ext] = Path;
            _pos++;
            if (_tokens[_pos].type == SEMICOLON)
                _pos++;
            else
            {
                error_msg(2);
                return 0;
            }
        }
        else
        {
            error_msg(4);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
}

int Parser::allow_methods_parse(int &_pos, LocationConfig &locConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        while (_tokens[_pos].type == WORD)
        {
            locConfig.allowMethods.push_back(_tokens[_pos].value);
            _pos++;
        }
        if (_tokens[_pos].type == SEMICOLON)
            _pos++;
        else
        {
            error_msg(2);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
}

int Parser::error_page_parse(int &_pos, ServerConfig &serverConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        int errorCode = atoi(_tokens[_pos].value.c_str());
        _pos++;
        if (_tokens[_pos].type == WORD)
        {
            std::string errorPath = _tokens[_pos].value;
            serverConfig.error_Pages[errorCode] = errorPath;
            _pos++;
            if (_tokens[_pos].type == SEMICOLON)
                _pos++;
            else
            {
                error_msg(2);
                return 0;
            }
        }
        else
        {
            error_msg(4);
            return 0;
        }
    }
    else
    {
        error_msg(4);
        return 0;
    }
    return 1;
}

LocationConfig Parser::location_parse(int &_pos)
{
    LocationConfig locConfig;
    if (_pos >= (int)_tokens.size() || _tokens[_pos].type != WORD)
    {
        error_msg(4);
        return locConfig;
    }

    locConfig.path = _tokens[_pos].value;
    _pos++;
    if (_pos >= (int)_tokens.size() || _tokens[_pos].type != L_BRACE)
    {
        error_msg(3);
        return locConfig;
    }
    _pos++;

    while (_pos < (int)_tokens.size() && _tokens[_pos].type != R_BRACE)
    {
        if (_tokens[_pos].type != WORD)
        {
            error_msg(4);
            return locConfig;
        }

        const std::string &key = _tokens[_pos].value;

        if (key == "autoindex" || key == "upload_enable")
        {
            if (!uploadEnable_and_autoindex_parse(_pos, locConfig))
                return locConfig;
        }
        else if (key == "return")
        {
            if (!return_parse(_pos, locConfig))
                return locConfig;
        }
        else if (key == "allow_methods")
        {
            if (!allow_methods_parse(_pos, locConfig))
                return locConfig;
        }
        else if (key == "upload_store")
        {
            if (!upload_parse(_pos, locConfig))
                return locConfig;
        }
        else if (key == "cgi_extension")
        {
            if (!cgi_extension_parse(_pos, locConfig))
                return locConfig;
        }
        else
        {
            error_msg(4);
            return locConfig;
        }
        if (_tokens[_pos].type == R_BRACE)
            break;
    }
    if (_pos >= (int)_tokens.size() || _tokens[_pos].type != R_BRACE)
    {
        error_msg(3);
        return locConfig;
    }
    _pos++;

    return locConfig;
}


ServerConfig Parser::server_parse(int &_pos)
{
    ServerConfig serverConfig;

    if (_pos >= (int)_tokens.size() || _tokens[_pos].type != L_BRACE)
    {
        error_msg(3);
        return serverConfig;
    }
    _pos++;

    while (_pos < (int)_tokens.size() && _tokens[_pos].type != R_BRACE)
    {
        if (_tokens[_pos].type != WORD)
        {
            error_msg(3);
            return serverConfig;
        }

        const std::string &key = _tokens[_pos].value;
        if (key == "location")
        {
            _pos++;
            LocationConfig loc = location_parse(_pos);
            serverConfig.locations.push_back(loc);
            if (_tokens[_pos].type == R_BRACE)
                continue ;
        }
        else if (key == "listen" || key == "client_max_body_size")
        {
            if (!port_and_clientMaxBodySize_parse(_pos, serverConfig))
                return serverConfig;
        }
        else if (key == "root" || key == "index")
        {
            if (!root_index_parse(_pos, serverConfig))
                return serverConfig;
        }
        else if (key == "error_page")
        {
            if (!error_page_parse(_pos, serverConfig))
                return serverConfig;
        }
        else
        {
            error_msg(3);
            return serverConfig;
        }
        if (_tokens[_pos].type == R_BRACE)
            break;
    }

    if (_pos >= (int)_tokens.size() || _tokens[_pos].type != R_BRACE)
    {
        error_msg(3);
        return serverConfig;
    }
    _pos++;

    return serverConfig;
}


Config    Parser::parse()
{
    Config conf;
    for (; _pos < static_cast<int>(_tokens.size()); )
    {
        if (_tokens[_pos].type == WORD && _tokens[_pos].value == "server")
        {
            _pos++;
            ServerConfig serv = server_parse(_pos);
            conf.servers.push_back(serv);
        }
        else
        {
            std::cerr << "Unexpected token at line " << _tokens[_pos].line << std::endl;
            _pos++;
        }
    }
    return conf;
}
