/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2007 INRIA
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: 
 */

#include "ns3/object.h"
#include "ns3/log.h"
#include "ns3/uinteger.h"
#include "ns3/integer.h"
#include "ns3/boolean.h"
#include "ns3/trace-source-accessor.h"
#include "udp-socket.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RudpSocket");

NS_OBJECT_ENSURE_REGISTERED (RudpSocket);

TypeId
RudpSocket::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RudpSocket")
    .SetParent<Socket> ()
    .SetGroupName ("Internet")
    .AddAttribute ("RcvBufSize",
                   "RudpSocket maximum receive buffer size (bytes)",
                   UintegerValue (131072),
                   MakeUintegerAccessor (&RudpSocket::GetRcvBufSize,
                                         &RudpSocket::SetRcvBufSize),
                   MakeUintegerChecker<uint32_t> ())
    .AddAttribute ("IpTtl",
                   "socket-specific TTL for unicast IP packets (if non-zero)",
                   UintegerValue (0),
                   MakeUintegerAccessor (&UdpSocket::GetIpTtl,
                                         &UdpSocket::SetIpTtl),
                   MakeUintegerChecker<uint8_t> ())
    .AddAttribute ("MtuDiscover", "If enabled, every outgoing ip packet will have the DF flag set.",
                   BooleanValue (false),
                   MakeBooleanAccessor (&UdpSocket::SetMtuDiscover,
                                        &UdpSocket::GetMtuDiscover),
                   MakeBooleanChecker ())
  ;
  return tid;
}

RudpSocket::RudpSocket ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

RudpSocket::~RudpSocket ()
{
  NS_LOG_FUNCTION_NOARGS ();
}

} // namespace ns3
