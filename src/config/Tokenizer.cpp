/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.cpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:36:32 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:36:32 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "Tokenizer.hpp"

Tokenizer::Tokenizer(const std::string& input) : _input(input) , _pos(0), _line(1) {
    // std::cout << "Tokenizer created" << std::endl;
}

Tokenizer::~Tokenizer() {
    // std::cout << "Tokenizer destroyed" << std::endl;
}

void Tokenizer::skip_sp(int &_pos)
{
    for (; _pos < static_cast<int>(_input.size())
            && (_input[_pos] == 32
               || _input[_pos] == '\t'
               || _input[_pos] == '\r'
               || _input[_pos] == '\n'); _pos++) 
    {
        if (_input[_pos] == '\n')
            _line++;
    }
}

std::vector<Token> Tokenizer::tokenize()
{
    std::vector<Token> tokens;
    for (; _pos < static_cast<int>(_input.size()); )
    {
        skip_sp(_pos);
        if (_pos >= static_cast<int>(_input.size()))
            break;

        if (_input[_pos] == '{')
        {
            Token t;
            t.line = _line;
            t.type = L_BRACE;
            t.value = "";
            tokens.push_back(t);
            _pos++;
        }
        else if (_input[_pos] == '}')
        {
            Token t;
            t.line = _line;
            t.type = R_BRACE;
            t.value = "";
            tokens.push_back(t);
            _pos++;
        }
        else if (_input[_pos] == ';')
        {
            Token t;
            t.line = _line;
            t.type = SEMICOLON;
            t.value = "";
            tokens.push_back(t);
            _pos++;
        }
        else
        {
            std::string word;
            while (_pos < static_cast<int>(_input.size())
                   && _input[_pos] != ' '
                   && _input[_pos] != '\t'
                   && _input[_pos] != '\r'
                   && _input[_pos] != '\n'
                   && _input[_pos] != '{'
                   && _input[_pos] != '}'
                   && _input[_pos] != ';')
            {
                word += _input[_pos];
                _pos++;
            }
            Token t;
            t.line = _line;
            t.type = WORD;
            t.value = word;
            tokens.push_back(t);
        }
    }
    return tokens;
}
