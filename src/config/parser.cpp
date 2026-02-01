/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.cpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:36:11 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:36:11 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Parser.hpp"

Parser::Parser(const std::vector<Token>& tokens) : _tokens(tokens), _pos(0), _fatal(false)
{
    // std::cout << "Parser created." << std::endl;
}

Parser::~Parser()
{
    // std::cout << "Parser destroyed." << std::endl;
}
void Parser::error_duplicate_port(int port, int line)
{
    _fatal = true;
    std::cerr << "Config Error: duplicate listen port " << port;
    if (line != -1)
        std::cerr << " at line " << line;
    std::cerr << std::endl;
}

void Parser::error_msg(int num)
{
    _fatal = true;

    int line = (_pos < (int)_tokens.size()) ? _tokens[_pos].line : -1;

    if (num == 2)
        std::cerr << "Syntax Error: Missing semicolon at line " << line << " in config file\n";
    else if (num == 3)
        std::cerr << "Syntax Error: Missing brace at line " << line << " in config file\n";
    else if (num == 4)
        std::cerr << "Syntax Error: Invalid directive at line " << line << " in config file\n";
    else
        std::cerr << "Syntax Error at line " << line << " in config file\n";
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
            if (_fatal)
                return Config();

            if (serv.port <= 0 || serv.port > 65535)
            {
                _fatal = true;
                std::cerr << "Config Error: invalid listen port " << serv.port << std::endl;
                return Config();
            }
            for (size_t i = 0; i < conf.servers.size(); i++)
            {
                if (conf.servers[i].port == serv.port)
                {
                    if (conf.servers[i].server_name == serv.server_name)
                    {
                        int line = (serv.listen_line != -1) ? serv.listen_line : -1;
                    
                        _fatal = true;
                        std::cerr << "Config Error: duplicate listen port "
                                  << serv.port
                                  << " with same server_name '" << serv.server_name << "'";
                        if (line != -1)
                            std::cerr << " at line " << line;
                        std::cerr << std::endl;
                    
                        return Config();
                    }
                }
            }


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
