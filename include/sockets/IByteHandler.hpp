/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   IByteHandler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:42:19 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:42:20 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef IBYTEHANDLER_HPP
#define IBYTEHANDLER_HPP

#include <string>

struct ByteReply {
    std::string bytes;
    bool        closeAfterWrite;

    ByteReply() : bytes(), closeAfterWrite(true) {}
    ByteReply(const std::string& b, bool c) : bytes(b), closeAfterWrite(c) {}
};

class IByteHandler
{
public:
    virtual ~IByteHandler() {}
    virtual ByteReply handleBytes(int acceptFd, const std::string& rawMessage) = 0;
};

#endif
