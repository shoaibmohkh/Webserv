/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   NetUtil.hpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: sal-kawa <sal-kawa@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/02/01 16:41:49 by sal-kawa          #+#    #+#             */
/*   Updated: 2026/02/01 16:41:49 by sal-kawa         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef NETUTIL_HPP
#define NETUTIL_HPP

#include <vector>
#include <string>

int  makeNonBlocking(int fd);
void closeFd(int fd);
bool parsePortsList(const char* s, std::vector<int>& outPorts);

#endif
