/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Tokenizer.hpp                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/14 12:02:56 by eaqrabaw          #+#    #+#             */
/*   Updated: 2025/12/14 12:02:56 by eaqrabaw         ###   ########.fr       */
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