/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   location_parser.cpp                                :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 12:03:24 by eaqrabaw          #+#    #+#             */
/*   Updated: 2025/12/14 12:03:24 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"

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

int Parser::location_root_parse(int &_pos, LocationConfig &locConfig)
{
    if (_pos + 1 >= (int)_tokens.size())
    {
        error_msg(4);
        return 0;
    }
    _pos++;
    if (_tokens[_pos].type == WORD)
    {
        locConfig.root = _tokens[_pos].value;
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
        else if (key == "root")
        {
            if (!location_root_parse(_pos, locConfig))
                return locConfig;
        }
        else
        {
            std::cerr << "Warning: Unknown location directive '" << key 
                      << "' at line " << _tokens[_pos].line << ", skipping..." << std::endl;
            skip_directive(_pos);
        }
        if (_pos < (int)_tokens.size() && _tokens[_pos].type == R_BRACE)
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