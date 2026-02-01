/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Http10Serializer.hpp                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:42:30 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:42:31 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP10SERIALIZER_HPP
#define HTTP10SERIALIZER_HPP

#include "../HttpResponse.hpp"
#include <string>

namespace http10
{
    std::string makeError(int code, const char* msg);
    std::string serializeClose(const HTTPResponse& res);
}

#endif
