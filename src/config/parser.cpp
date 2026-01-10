/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 12:03:36 by eaqrabaw          #+#    #+#             */
/*   Updated: 2025/12/14 12:03:36 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"

Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens), _pos(0)
{
    // std::cout << "Parser created." << std::endl;
}

Parser::~Parser()
{
    // std::cout << "Parser destroyed." << std::endl;
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

void Parser::skip_directive(int &_pos)
{
    while (_pos < static_cast<int>(_tokens.size()) 
           && _tokens[_pos].type != SEMICOLON 
           && _tokens[_pos].type != R_BRACE)
    {
        _pos++;
    }
    if (_pos < static_cast<int>(_tokens.size()) && _tokens[_pos].type == SEMICOLON)
        _pos++;
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
