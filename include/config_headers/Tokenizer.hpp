/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   tokenizer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 18:09:11 by marvin            #+#    #+#             */
/*   Updated: 2025/12/10 18:09:11 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef TOKENIZER_HPP
#define TOKENIZER_HPP

#include <string>
#include <iostream>
#include <vector>

enum Token_type {
    WORD,
    L_BRACE,
    R_BRACE,
    SEMICOLON,
};

struct Token {
    int         line;  // line number in the config file
    Token_type  type;
    std::string value; // just for word and Main_Word types
};

class Tokenizer {
    private:
        std::string             _input;
        int                     _pos;
        int                     _line;

        void                    skip_sp(int &_pos);


    public:
        Tokenizer(const std::string& input);
        ~Tokenizer();

        std::vector<Token>  tokenize();
};

#endif