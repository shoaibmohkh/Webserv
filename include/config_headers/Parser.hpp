/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   parser.hpp                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: marvin <marvin@student.42.fr>              +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/12/10 20:21:56 by marvin            #+#    #+#             */
/*   Updated: 2025/12/10 20:21:56 by marvin           ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef PARSER_HPP
#define PARSER_HPP

#include "Tokenizer.hpp"
#include "Config.hpp"
#include <cstdlib>

class Parser {
    private:
        std::vector<Token>     _tokens;
        int                    _pos;
        LocationConfig         location_parse(int &_pos);
        ServerConfig           server_parse(int &_pos);
        void                   error_msg(int num);
        void                   skip_directive(int &_pos);
        int                    uploadEnable_and_autoindex_parse(int &_pos, LocationConfig &locConfig);
        int                    port_and_clientMaxBodySize_parse(int &_pos, ServerConfig &serverConfig);
        int                    return_parse(int &_pos, LocationConfig &locConfig);
        int                    upload_parse(int &_pos, LocationConfig &locConfig);
        int                    root_index_parse(int &_pos, ServerConfig &serverConfig);
        int                    location_root_parse(int &_pos, LocationConfig &locConfig);
        int                    server_name_parse(int &_pos, ServerConfig &serverConfig);
        int                    cgi_extension_parse(int &_pos, LocationConfig &locConfig);
        int                    allow_methods_parse(int &_pos, LocationConfig &locConfig);  
        int                    error_page_parse(int &_pos, ServerConfig &serverConfig);
    public:
        Parser(const std::vector<Token>& tokens);
        ~Parser();

        Config    parse();    
};

#endif