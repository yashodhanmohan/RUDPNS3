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
#include "rudp-socket-factory-impl.h"
#include "rudp-l4-protocol.h"
#include "ns3/socket.h"
#include "ns3/assert.h"

namespace ns3 {

RudpSocketFactoryImpl::RudpSocketFactoryImpl ()
  : m_rudp (0)
{
}
RudpSocketFactoryImpl::~RudpSocketFactoryImpl ()
{
  NS_ASSERT (m_rudp == 0);
}

void
RudpSocketFactoryImpl::SetRudp (Ptr<RudpL4Protocol> rudp)
{
  m_rudp = rudp;
}

Ptr<Socket>
RudpSocketFactoryImpl::CreateSocket (void)
{
  return m_rudp->CreateSocket ();
}

void 
RudpSocketFactoryImpl::DoDispose (void)
{
  m_rudp = 0;
  RudpSocketFactory::DoDispose ();
}

} // namespace ns3
