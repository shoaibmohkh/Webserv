/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Http10Parser.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:42:32 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:42:33 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP10PARSER_HPP
#define HTTP10PARSER_HPP

#include "../HttpRequest.hpp"
#include <string>

namespace http10
{

    bool parseRequest(const std::string& raw,
                      int listenPort,
                      HTTPRequest& outReq,
                      int& outErrCode);
}

#endif
