/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   server_parser.cpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 12:03:44 by eaqrabaw          #+#    #+#             */
/*   Updated: 2025/12/14 12:03:44 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"

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
{
    serverConfig.port = atoi(_tokens[_pos].value.c_str());
    serverConfig.listen_line = _tokens[_pos].line;  // <-- add this
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

int Parser::server_name_parse(int &_pos, ServerConfig &serverConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        serverConfig.server_name = _tokens[_pos].value;
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
            error_msg(4);
            skip_directive(_pos);
            continue;
        }

        const std::string &key = _tokens[_pos].value;

        if (key == "location")
        {
            _pos++;
            LocationConfig loc = location_parse(_pos);
            serverConfig.locations.push_back(loc);
            continue;
        }
        else if (key == "listen" || key == "client_max_body_size")
        {
            if (!port_and_clientMaxBodySize_parse(_pos, serverConfig))
                skip_directive(_pos);
        }
        else if (key == "root" || key == "index")
        {
            if (!root_index_parse(_pos, serverConfig))
                return serverConfig;
        }
        else if (key == "server_name")
        {
            if (!server_name_parse(_pos, serverConfig))
                skip_directive(_pos);
        }
        else if (key == "error_page")
        {
            if (!error_page_parse(_pos, serverConfig))
                skip_directive(_pos);
        }
        else
        {
            std::cerr << "Warning: Unknown server directive '" << key
                      << "' at line " << _tokens[_pos].line
                      << ", skipping...\n";
            skip_directive(_pos);
        }
    }

    if (_pos >= (int)_tokens.size() || _tokens[_pos].type != R_BRACE)
    {
        error_msg(3);
        return serverConfig;
    }

    _pos++;
    return serverConfig;
}