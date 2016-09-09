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

#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/ipv4-route.h"
#include "ns3/ipv6-route.h"
#include "ns3/ipv4.h"
#include "ns3/ipv6.h"
#include "ns3/ipv6-l3-protocol.h"
#include "ns3/ipv4-header.h"
#include "ns3/ipv4-routing-protocol.h"
#include "ns3/ipv6-routing-protocol.h"
#include "ns3/rudp-socket-factory.h"
#include "ns3/trace-source-accessor.h"
#include "ns3/ipv4-packet-info-tag.h"
#include "ns3/ipv6-packet-info-tag.h"
#include "rudp-socket-impl.h"
#include "rudp-l4-protocol.h"
#include "ipv4-end-point.h"
#include "ipv6-end-point.h"
#include <limits>

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RudpSocketImpl");

NS_OBJECT_ENSURE_REGISTERED (RudpSocketImpl);

// The correct maximum UDP message size is 65507, as determined by the following formula:
// 0xffff - (sizeof(IP Header) + sizeof(UDP Header)) = 65535-(20+8) = 65507
// \todo MAX_IPV4_UDP_DATAGRAM_SIZE is correct only for IPv4
static const uint32_t MAX_IPV4_RUDP_DATAGRAM_SIZE = 65507; //!< Maximum RUDP datagram size

// Add attributes generic to all UdpSockets to base class UdpSocket
TypeId
RudpSocketImpl::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RudpSocketImpl")
    .SetParent<RudpSocket> ()
    .SetGroupName ("Internet")
    .AddConstructor<RudpSocketImpl> ()
    .AddTraceSource ("Drop",
                     "Drop RUDP packet due to receive buffer overflow",
                     MakeTraceSourceAccessor (&RudpSocketImpl::m_dropTrace),
                     "ns3::Packet::TracedCallback")
    .AddAttribute ("IcmpCallback", "Callback invoked whenever an icmp error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&RudpSocketImpl::m_icmpCallback),
                   MakeCallbackChecker ())
    .AddAttribute ("IcmpCallback6", "Callback invoked whenever an icmpv6 error is received on this socket.",
                   CallbackValue (),
                   MakeCallbackAccessor (&RudpSocketImpl::m_icmpCallback6),
                   MakeCallbackChecker ())
  ;
  return tid;
}

RudpSocketImpl::RudpSocketImpl ()
  : m_endPoint (0),
    m_endPoint6 (0),
    m_node (0),
    m_udp (0),
    m_errno (ERROR_NOTERROR),
    m_shutdownSend (false),
    m_shutdownRecv (false),
    m_connected (false),
    m_rxAvailable (0)
{
  NS_LOG_FUNCTION_NOARGS ();
}

RudpSocketImpl::~RudpSocketImpl ()
{
  NS_LOG_FUNCTION_NOARGS ();

  m_node = 0;
  /**
   * Note: actually this function is called AFTER
   * UdpSocketImpl::Destroy or UdpSocketImpl::Destroy6
   * so the code below is unnecessary in normal operations
   */
  if (m_endPoint != 0)
    {
      NS_ASSERT (m_rudp != 0);
      /**
       * Note that this piece of code is a bit tricky:
       * when DeAllocate is called, it will call into
       * Ipv4EndPointDemux::Deallocate which triggers
       * a delete of the associated endPoint which triggers
       * in turn a call to the method UdpSocketImpl::Destroy below
       * will will zero the m_endPoint field.
       */
      NS_ASSERT (m_endPoint != 0);
      m_rudp->DeAllocate (m_endPoint);
      NS_ASSERT (m_endPoint == 0);
    }
  if (m_endPoint6 != 0)
    {
      NS_ASSERT (m_rudp != 0);
      /**
       * Note that this piece of code is a bit tricky:
       * when DeAllocate is called, it will call into
       * Ipv4EndPointDemux::Deallocate which triggers
       * a delete of the associated endPoint which triggers
       * in turn a call to the method UdpSocketImpl::Destroy below
       * will will zero the m_endPoint field.
       */
      NS_ASSERT (m_endPoint6 != 0);
      m_rudp->DeAllocate (m_endPoint6);
      NS_ASSERT (m_endPoint6 == 0);
    }
  m_rudp = 0;
}

void 
RudpSocketImpl::SetNode (Ptr<Node> node)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_node = node;
}

void 
RudpSocketImpl::SetRudp (Ptr<RudpL4Protocol> rudp)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_rudp = rudp;
}


enum Socket::SocketErrno
RudpSocketImpl::GetErrno (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_errno;
}

enum Socket::SocketType
RudpSocketImpl::GetSocketType (void) const
{
  return NS3_SOCK_DGRAM;
}

Ptr<Node>
RudpSocketImpl::GetNode (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  return m_node;
}

void 
RudpSocketImpl::Destroy (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_endPoint = 0;
}

void
RudpSocketImpl::Destroy6 (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_endPoint6 = 0;
}

/* Deallocate the end point and cancel all the timers */
void
RudpSocketImpl::DeallocateEndPoint (void)
{
  if (m_endPoint != 0)
    {
      m_endPoint->SetDestroyCallback (MakeNullCallback<void> ());
      m_rudp->DeAllocate (m_endPoint);
      m_endPoint = 0;
    }
  if (m_endPoint6 != 0)
    {
      m_endPoint6->SetDestroyCallback (MakeNullCallback<void> ());
      m_rudp->DeAllocate (m_endPoint6);
      m_endPoint6 = 0;
    }
}

int
RudpSocketImpl::FinishBind (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  bool done = false;
  if (m_endPoint != 0)
    {
      m_endPoint->SetRxCallback (MakeCallback (&RudpSocketImpl::ForwardUp, Ptr<RudpSocketImpl> (this)));
      m_endPoint->SetIcmpCallback (MakeCallback (&RudpSocketImpl::ForwardIcmp, Ptr<RudpSocketImpl> (this)));
      m_endPoint->SetDestroyCallback (MakeCallback (&RudpSocketImpl::Destroy, Ptr<RudpSocketImpl> (this)));
      done = true;
    }
  if (m_endPoint6 != 0)
    {
      m_endPoint6->SetRxCallback (MakeCallback (&RudpSocketImpl::ForwardUp6, Ptr<RudpSocketImpl> (this)));
      m_endPoint6->SetIcmpCallback (MakeCallback (&RudpSocketImpl::ForwardIcmp6, Ptr<RudpSocketImpl> (this)));
      m_endPoint6->SetDestroyCallback (MakeCallback (&RudpSocketImpl::Destroy6, Ptr<RudpSocketImpl> (this)));
      done = true;
    }
  if (done)
    {
      return 0;
    }
  return -1;
}

int
RudpSocketImpl::Bind (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_endPoint = m_rudp->Allocate ();
  return FinishBind ();
}

int
RudpSocketImpl::Bind6 (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_endPoint6 = m_rudp->Allocate6 ();
  return FinishBind ();
}

int 
RudpSocketImpl::Bind (const Address &address)
{
  NS_LOG_FUNCTION (this << address);

  if (InetSocketAddress::IsMatchingType (address))
    {
      NS_ASSERT_MSG (m_endPoint == 0, "Endpoint already allocated (maybe you used BindToNetDevice before Bind).");

      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      Ipv4Address ipv4 = transport.GetIpv4 ();
      uint16_t port = transport.GetPort ();
      if (ipv4 == Ipv4Address::GetAny () && port == 0)
        {
          m_endPoint = m_rudp->Allocate ();
        }
      else if (ipv4 == Ipv4Address::GetAny () && port != 0)
        {
          m_endPoint = m_rudp->Allocate (port);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port == 0)
        {
          m_endPoint = m_rudp->Allocate (ipv4);
        }
      else if (ipv4 != Ipv4Address::GetAny () && port != 0)
        {
          m_endPoint = m_rudp->Allocate (ipv4, port);
        }
      if (0 == m_endPoint)
        {
          m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
          return -1;
        }
    }
  else if (Inet6SocketAddress::IsMatchingType (address))
    {
      NS_ASSERT_MSG (m_endPoint == 0, "Endpoint already allocated (maybe you used BindToNetDevice before Bind).");

      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address ipv6 = transport.GetIpv6 ();
      uint16_t port = transport.GetPort ();
      if (ipv6 == Ipv6Address::GetAny () && port == 0)
        {
          m_endPoint6 = m_rudp->Allocate6 ();
        }
      else if (ipv6 == Ipv6Address::GetAny () && port != 0)
        {
          m_endPoint6 = m_rudp->Allocate6 (port);
        }
      else if (ipv6 != Ipv6Address::GetAny () && port == 0)
        {
          m_endPoint6 = m_rudp->Allocate6 (ipv6);
        }
      else if (ipv6 != Ipv6Address::GetAny () && port != 0)
        {
          m_endPoint6 = m_rudp->Allocate6 (ipv6, port);
        }
      if (0 == m_endPoint6)
        {
          m_errno = port ? ERROR_ADDRINUSE : ERROR_ADDRNOTAVAIL;
          return -1;
        }
    }
  else
    {
      NS_LOG_ERROR ("Not IsMatchingType");
      m_errno = ERROR_INVAL;
      return -1;
    }

  return FinishBind ();
}

int 
RudpSocketImpl::ShutdownSend (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_shutdownSend = true;
  return 0;
}

int 
RudpSocketImpl::ShutdownRecv (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_shutdownRecv = true;
  if (m_endPoint)
    {
      m_endPoint->SetRxEnabled (false);
    }
  if (m_endPoint6)
    {
      m_endPoint6->SetRxEnabled (false);
    }
  return 0;
}

int
RudpSocketImpl::Close (void)
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_shutdownRecv == true && m_shutdownSend == true)
    {
      m_errno = Socket::ERROR_BADF;
      return -1;
    }
  Ipv6LeaveGroup ();
  m_shutdownRecv = true;
  m_shutdownSend = true;
  DeallocateEndPoint ();
  return 0;
}

int
RudpSocketImpl::Connect (const Address & address)
{
  NS_LOG_FUNCTION (this << address);
  if (InetSocketAddress::IsMatchingType(address) == true)
    {
      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      m_defaultAddress = Address(transport.GetIpv4 ());
      m_defaultPort = transport.GetPort ();
      m_connected = true;
      NotifyConnectionSucceeded ();
    }
  else if (Inet6SocketAddress::IsMatchingType(address) == true)
    {
      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      m_defaultAddress = Address(transport.GetIpv6 ());
      m_defaultPort = transport.GetPort ();
      m_connected = true;
      NotifyConnectionSucceeded ();
    }
  else
    {
      return -1;
    }

  return 0;
}

int 
RudpSocketImpl::Listen (void)
{
  m_errno = Socket::ERROR_OPNOTSUPP;
  return -1;
}

int 
RudpSocketImpl::Send (Ptr<Packet> p, uint32_t flags)
{
  NS_LOG_FUNCTION (this << p << flags);
  // Check if socket is connected
  if (!m_connected)
    {
      m_errno = ERROR_NOTCONN;
      return -1;
    }

  return DoSend (p);
}

int 
RudpSocketImpl::DoSend (Ptr<Packet> p)
{
  NS_LOG_FUNCTION (this << p);
  if ((m_endPoint == 0) && (InetSocketAddress::IsMatchingType(m_defaultAddress) == true))
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return -1;
        }
      NS_ASSERT (m_endPoint != 0);
    }
  else if ((m_endPoint6 == 0) && (Inet6SocketAddress::IsMatchingType(m_defaultAddress) == true))
    {
      if (Bind6 () == -1)
        {
          NS_ASSERT (m_endPoint6 == 0);
          return -1;
        }
      NS_ASSERT (m_endPoint6 != 0);
    }
  if (m_shutdownSend)
    {
      m_errno = ERROR_SHUTDOWN;
      return -1;
    } 

  return DoSendTo (p, (const Address)m_defaultAddress);
}

int
RudpSocketImpl::DoSendTo (Ptr<Packet> p, const Address &address)
{
  NS_LOG_FUNCTION (this << p << address);

  if (!m_connected)
    {
      NS_LOG_LOGIC ("Not connected");
      if (InetSocketAddress::IsMatchingType(address) == true)
        {
          InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
          Ipv4Address ipv4 = transport.GetIpv4 ();
          uint16_t port = transport.GetPort ();
          return DoSendTo (p, ipv4, port);
        }
      else if (Inet6SocketAddress::IsMatchingType(address) == true)
        {
          Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
          Ipv6Address ipv6 = transport.GetIpv6 ();
          uint16_t port = transport.GetPort ();
          return DoSendTo (p, ipv6, port);
        }
      else
        {
          return -1;
        }
    }
  else
    {
      // connected UDP socket must use default addresses
      NS_LOG_LOGIC ("Connected");
      if (Ipv4Address::IsMatchingType(m_defaultAddress))
        {
          return DoSendTo (p, Ipv4Address::ConvertFrom(m_defaultAddress), m_defaultPort);
        }
      else if (Ipv6Address::IsMatchingType(m_defaultAddress))
        {
          return DoSendTo (p, Ipv6Address::ConvertFrom(m_defaultAddress), m_defaultPort);
        }
    }
  m_errno = ERROR_AFNOSUPPORT;
  return(-1);
}

int
RudpSocketImpl::DoSendTo (Ptr<Packet> p, Ipv4Address dest, uint16_t port)
{
  NS_LOG_FUNCTION (this << p << dest << port);
  if (m_boundnetdevice)
    {
      NS_LOG_LOGIC ("Bound interface number " << m_boundnetdevice->GetIfIndex ());
    }
  if (m_endPoint == 0)
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return -1;
        }
      NS_ASSERT (m_endPoint != 0);
    }
  if (m_shutdownSend)
    {
      m_errno = ERROR_SHUTDOWN;
      return -1;
    }

  if (p->GetSize () > GetTxAvailable () )
    {
      m_errno = ERROR_MSGSIZE;
      return -1;
    }

  if (IsManualIpTos ())
    {
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (GetIpTos ());
      p->AddPacketTag (ipTosTag);
    }

  Ptr<Ipv4> ipv4 = m_node->GetObject<Ipv4> ();

  if (IsManualIpTtl () && GetIpTtl () != 0 && !dest.IsMulticast () && !dest.IsBroadcast ())
    {
      SocketIpTtlTag tag;
      tag.SetTtl (GetIpTtl ());
      p->AddPacketTag (tag);
    }
  {
    SocketSetDontFragmentTag tag;
    bool found = p->RemovePacketTag (tag);
    if (!found)
      {
        if (m_mtuDiscover)
          {
            tag.Enable ();
          }
        else
          {
            tag.Disable ();
          }
        p->AddPacketTag (tag);
      }
  }
  
  if (m_endPoint->GetLocalAddress () != Ipv4Address::GetAny ())
    {
      m_rudp->Send (p->Copy (), m_endPoint->GetLocalAddress (), dest,
                   m_endPoint->GetLocalPort (), port, 0);
      NotifyDataSent (p->GetSize ());
      NotifySend (GetTxAvailable ());
      return p->GetSize ();
    }
  else if (ipv4->GetRoutingProtocol () != 0)
    {
      Ipv4Header header;
      header.SetDestination (dest);
      header.SetProtocol (UdpL4Protocol::PROT_NUMBER);
      Socket::SocketErrno errno_;
      Ptr<Ipv4Route> route;
      Ptr<NetDevice> oif = m_boundnetdevice; //specify non-zero if bound to a specific device
      // TBD-- we could cache the route and just check its validity
      route = ipv4->GetRoutingProtocol ()->RouteOutput (p, header, oif, errno_); 
      if (route != 0)
        {
          NS_LOG_LOGIC ("Route exists");

          header.SetSource (route->GetSource ());
          m_rudp->Send (p->Copy (), header.GetSource (), header.GetDestination (),
                       m_endPoint->GetLocalPort (), port, route);
          NotifyDataSent (p->GetSize ());
          return p->GetSize ();
        }
      else 
        {
          NS_LOG_LOGIC ("No route to destination");
          NS_LOG_ERROR (errno_);
          m_errno = errno_;
          return -1;
        }
    }
  else
    {
      NS_LOG_ERROR ("ERROR_NOROUTETOHOST");
      m_errno = ERROR_NOROUTETOHOST;
      return -1;
    }

  return 0;
}

int
RudpSocketImpl::DoSendTo (Ptr<Packet> p, Ipv6Address dest, uint16_t port)
{
  NS_LOG_FUNCTION (this << p << dest << port);

  if (dest.IsIpv4MappedAddress ())
    {
        return (DoSendTo(p, dest.GetIpv4MappedAddress (), port));
    }
  if (m_boundnetdevice)
    {
      NS_LOG_LOGIC ("Bound interface number " << m_boundnetdevice->GetIfIndex ());
    }
  if (m_endPoint6 == 0)
    {
      if (Bind6 () == -1)
        {
          NS_ASSERT (m_endPoint6 == 0);
          return -1;
        }
      NS_ASSERT (m_endPoint6 != 0);
    }
  if (m_shutdownSend)
    {
      m_errno = ERROR_SHUTDOWN;
      return -1;
    }

  if (p->GetSize () > GetTxAvailable () )
    {
      m_errno = ERROR_MSGSIZE;
      return -1;
    }

    if (IsManualIpv6Tclass ())
    {
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (GetIpv6Tclass ());
      p->AddPacketTag (ipTclassTag);
    }

  Ptr<Ipv6> ipv6 = m_node->GetObject<Ipv6> ();

  
  if (IsManualIpv6HopLimit () && GetIpv6HopLimit () != 0 && !dest.IsMulticast ())
    {
      SocketIpv6HopLimitTag tag;
      tag.SetHopLimit (GetIpv6HopLimit ());
      p->AddPacketTag (tag);
    }

  if (m_endPoint6->GetLocalAddress () != Ipv6Address::GetAny ())
    {
      m_rudp->Send (p->Copy (), m_endPoint6->GetLocalAddress (), dest,
                   m_endPoint6->GetLocalPort (), port, 0);
      NotifyDataSent (p->GetSize ());
      NotifySend (GetTxAvailable ());
      return p->GetSize ();
    }
  else if (ipv6->GetRoutingProtocol () != 0)
    {
      Ipv6Header header;
      header.SetDestinationAddress (dest);
      header.SetNextHeader (UdpL4Protocol::PROT_NUMBER);
      Socket::SocketErrno errno_;
      Ptr<Ipv6Route> route;
      Ptr<NetDevice> oif = m_boundnetdevice; //specify non-zero if bound to a specific device
      // TBD-- we could cache the route and just check its validity
      route = ipv6->GetRoutingProtocol ()->RouteOutput (p, header, oif, errno_); 
      if (route != 0)
        {
          NS_LOG_LOGIC ("Route exists");
          header.SetSourceAddress (route->GetSource ());
          m_rudp->Send (p->Copy (), header.GetSourceAddress (), header.GetDestinationAddress (),
                       m_endPoint6->GetLocalPort (), port, route);
          NotifyDataSent (p->GetSize ());
          return p->GetSize ();
        }
      else 
        {
          NS_LOG_LOGIC ("No route to destination");
          NS_LOG_ERROR (errno_);
          m_errno = errno_;
          return -1;
        }
    }
  else
    {
      NS_LOG_ERROR ("ERROR_NOROUTETOHOST");
      m_errno = ERROR_NOROUTETOHOST;
      return -1;
    }

  return 0;
}

uint32_t
RudpSocketImpl::GetTxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // No finite send buffer is modelled, but we must respect
  // the maximum size of an IP datagram (65535 bytes - headers).
  return MAX_IPV4_RUDP_DATAGRAM_SIZE;
}

int 
RudpSocketImpl::SendTo (Ptr<Packet> p, uint32_t flags, const Address &address)
{
  NS_LOG_FUNCTION (this << p << flags << address);
  if (InetSocketAddress::IsMatchingType (address))
    {
      if (IsManualIpTos ())
        {
          SocketIpTosTag ipTosTag;
          ipTosTag.SetTos (GetIpTos ());
          p->AddPacketTag (ipTosTag);
        }

      InetSocketAddress transport = InetSocketAddress::ConvertFrom (address);
      Ipv4Address ipv4 = transport.GetIpv4 ();
      uint16_t port = transport.GetPort ();
      return DoSendTo (p, ipv4, port);
    }
  else if (Inet6SocketAddress::IsMatchingType (address))
    {
      if (IsManualIpv6Tclass ())
        {
          SocketIpv6TclassTag ipTclassTag;
          ipTclassTag.SetTclass (GetIpv6Tclass ());
          p->AddPacketTag (ipTclassTag);
        }

      Inet6SocketAddress transport = Inet6SocketAddress::ConvertFrom (address);
      Ipv6Address ipv6 = transport.GetIpv6 ();
      uint16_t port = transport.GetPort ();
      return DoSendTo (p, ipv6, port);
    }
  return -1;
}

uint32_t
RudpSocketImpl::GetRxAvailable (void) const
{
  NS_LOG_FUNCTION_NOARGS ();
  // We separately maintain this state to avoid walking the queue 
  // every time this might be called
  return m_rxAvailable;
}

Ptr<Packet>
RudpSocketImpl::Recv (uint32_t maxSize, uint32_t flags)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  if (m_deliveryQueue.empty () )
    {
      m_errno = ERROR_AGAIN;
      return 0;
    }
  Ptr<Packet> p = m_deliveryQueue.front ();
  if (p->GetSize () <= maxSize) 
    {
      m_deliveryQueue.pop ();
      m_rxAvailable -= p->GetSize ();
    }
  else
    {
      p = 0; 
    }
  return p;
}

Ptr<Packet>
RudpSocketImpl::RecvFrom (uint32_t maxSize, uint32_t flags, 
                         Address &fromAddress)
{
  NS_LOG_FUNCTION (this << maxSize << flags);
  Ptr<Packet> packet = Recv (maxSize, flags);
  if (packet != 0)
    {
      SocketAddressTag tag;
      bool found;
      found = packet->PeekPacketTag (tag);
      NS_ASSERT (found);
      fromAddress = tag.GetAddress ();
    }
  return packet;
}

int
RudpSocketImpl::GetSockName (Address &address) const
{
  NS_LOG_FUNCTION_NOARGS ();
  if (m_endPoint != 0)
    {
      address = InetSocketAddress (m_endPoint->GetLocalAddress (), m_endPoint->GetLocalPort ());
    }
  else if (m_endPoint6 != 0)
    {
      address = Inet6SocketAddress (m_endPoint6->GetLocalAddress (), m_endPoint6->GetLocalPort ());
    }
  else
    { // It is possible to call this method on a socket without a name
      // in which case, behavior is unspecified
      // Should this return an InetSocketAddress or an Inet6SocketAddress?
      address = InetSocketAddress (Ipv4Address::GetZero (), 0);
    }
  return 0;
}

int
RudpSocketImpl::GetPeerName (Address &address) const
{
  NS_LOG_FUNCTION (this << address);

  if (!m_connected)
    {
      m_errno = ERROR_NOTCONN;
      return -1;
    }

  if (Ipv4Address::IsMatchingType (m_defaultAddress))
    {
      Ipv4Address addr = Ipv4Address::ConvertFrom (m_defaultAddress);
      address = InetSocketAddress (addr, m_defaultPort);
    }
  else if (Ipv6Address::IsMatchingType (m_defaultAddress))
    {
      Ipv6Address addr = Ipv6Address::ConvertFrom (m_defaultAddress);
      address = Inet6SocketAddress (addr, m_defaultPort);
    }
  else
    {
      NS_ASSERT_MSG (false, "unexpected address type");
    }

  return 0;
}

void
RudpSocketImpl::BindToNetDevice (Ptr<NetDevice> netdevice)
{
  NS_LOG_FUNCTION (netdevice);

  Socket::BindToNetDevice (netdevice); // Includes sanity check
  if (m_endPoint == 0)
    {
      if (Bind () == -1)
        {
          NS_ASSERT (m_endPoint == 0);
          return;
        }
      NS_ASSERT (m_endPoint != 0);
    }
  m_endPoint->BindToNetDevice (netdevice);

  if (m_endPoint6 == 0)
    {
      if (Bind6 () == -1)
        {
          NS_ASSERT (m_endPoint6 == 0);
          return;
        }
      NS_ASSERT (m_endPoint6 != 0);
    }
  m_endPoint6->BindToNetDevice (netdevice);

  return;
}

void 
RudpSocketImpl::ForwardUp (Ptr<Packet> packet, Ipv4Header header, uint16_t port,
                          Ptr<Ipv4Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header << port);

  if (m_shutdownRecv)
    {
      return;
    }

  // Should check via getsockopt ()..
  if (IsRecvPktInfo ())
    {
      Ipv4PacketInfoTag tag;
      packet->RemovePacketTag (tag);
      tag.SetRecvIf (incomingInterface->GetDevice ()->GetIfIndex ());
      packet->AddPacketTag (tag);
    }

  //Check only version 4 options
  if (IsIpRecvTos ())
    {
      SocketIpTosTag ipTosTag;
      ipTosTag.SetTos (header.GetTos ());
      packet->AddPacketTag (ipTosTag);
    }

  if (IsIpRecvTtl ())
    {
      SocketIpTtlTag ipTtlTag;
      ipTtlTag.SetTtl (header.GetTtl ());
      packet->AddPacketTag (ipTtlTag);
    }

  if ((m_rxAvailable + packet->GetSize ()) <= m_rcvBufSize)
    {
      Address address = InetSocketAddress (header.GetSource (), port);
      SocketAddressTag tag;
      tag.SetAddress (address);
      packet->AddPacketTag (tag);
      m_deliveryQueue.push (packet);
      m_rxAvailable += packet->GetSize ();
      NotifyDataRecv ();
    }
  else
    {
      // In general, this case should not occur unless the
      // receiving application reads data from this socket slowly
      // in comparison to the arrival rate
      //
      // drop and trace packet
      NS_LOG_WARN ("No receive buffer space available.  Drop.");
      m_dropTrace (packet);
    }
}

void 
RudpSocketImpl::ForwardUp6 (Ptr<Packet> packet, Ipv6Header header, uint16_t port, Ptr<Ipv6Interface> incomingInterface)
{
  NS_LOG_FUNCTION (this << packet << header.GetSourceAddress () << port);

  if (m_shutdownRecv)
    {
      return;
    }

  // Should check via getsockopt ()..
  if (IsRecvPktInfo ())
    {
      Ipv6PacketInfoTag tag;
      packet->RemovePacketTag (tag);
      tag.SetRecvIf (incomingInterface->GetDevice ()->GetIfIndex ());
      packet->AddPacketTag (tag);
    }

  //Check only version 6 options
  if (IsIpv6RecvTclass ())
    {
      SocketIpv6TclassTag ipTclassTag;
      ipTclassTag.SetTclass (header.GetTrafficClass ());
      packet->AddPacketTag (ipTclassTag);
    }

  if (IsIpv6RecvHopLimit ())
    {
      SocketIpv6HopLimitTag ipHopLimitTag;
      ipHopLimitTag.SetHopLimit (header.GetHopLimit ());
      packet->AddPacketTag (ipHopLimitTag);
    }

  if ((m_rxAvailable + packet->GetSize ()) <= m_rcvBufSize)
    {
      Address address = Inet6SocketAddress (header.GetSourceAddress (), port);
      SocketAddressTag tag;
      tag.SetAddress (address);
      packet->AddPacketTag (tag);
      m_deliveryQueue.push (packet);
      m_rxAvailable += packet->GetSize ();
      NotifyDataRecv ();
    }
  else
    {
      // In general, this case should not occur unless the
      // receiving application reads data from this socket slowly
      // in comparison to the arrival rate
      //
      // drop and trace packet
      NS_LOG_WARN ("No receive buffer space available.  Drop.");
      m_dropTrace (packet);
    }
}

void
RudpSocketImpl::ForwardIcmp (Ipv4Address icmpSource, uint8_t icmpTtl, 
                            uint8_t icmpType, uint8_t icmpCode,
                            uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  if (!m_icmpCallback.IsNull ())
    {
      m_icmpCallback (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}

void
RudpSocketImpl::ForwardIcmp6 (Ipv6Address icmpSource, uint8_t icmpTtl, 
                            uint8_t icmpType, uint8_t icmpCode,
                            uint32_t icmpInfo)
{
  NS_LOG_FUNCTION (this << icmpSource << (uint32_t)icmpTtl << (uint32_t)icmpType <<
                   (uint32_t)icmpCode << icmpInfo);
  if (!m_icmpCallback6.IsNull ())
    {
      m_icmpCallback6 (icmpSource, icmpTtl, icmpType, icmpCode, icmpInfo);
    }
}

void 
RudpSocketImpl::SetRcvBufSize (uint32_t size)
{
  m_rcvBufSize = size;
}

uint32_t 
RudpSocketImpl::GetRcvBufSize (void) const
{
  return m_rcvBufSize;
}

void 
RudpSocketImpl::SetMtuDiscover (bool discover)
{
  m_mtuDiscover = discover;
}

bool 
RudpSocketImpl::GetMtuDiscover (void) const
{
  return m_mtuDiscover;
}

} // namespace ns3
