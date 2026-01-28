/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Logger.h                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: eaqrabaw <eaqrabaw@student.42amman.com>    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/01/07 08:13:16 by eaqrabaw          #+#    #+#             */
/*   Updated: 2026/01/07 08:19:58 by eaqrabaw         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <iostream>

enum LogLevel {
    DEBUG,
    INFO,
    ERROR
};

class Logger
{
public:
	void static log(LogLevel level,const char* function , const std::string& message);
};

