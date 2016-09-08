/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2005 INRIA
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
 * Author: Mathieu Lacage <mathieu.lacage@sophia.inria.fr>
 */

#include "rudp-header.h"
#include "ns3/address-utils.h"

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED (RudpHeader);

/* The magic values below are used only for debugging.
 * They can be used to easily detect memory corruption
 * problems so you can see the patterns in memory.
 */
RudpHeader::RudpHeader ()
  : m_sourcePort (0xfffd),
    m_destinationPort (0xfffd),
    m_payloadSize (0),
{
}

RudpHeader::~RudpHeader ()
{
  m_sourcePort = 0xfffe;
  m_destinationPort = 0xfffe;
  m_payloadSize = 0xfffe;
}

void 
RudpHeader::SetDestinationPort (uint16_t port)
{
  m_destinationPort = port;
}
void 
RudpHeader::SetSourcePort (uint16_t port)
{
  m_sourcePort = port;
}
void
RudpHeader::SetControlFlag (bool controlFlag)
{
  m_controlFlag = controlFlag;
}
void
RudpHeader::SetPositionFlag (uint8_t positionFlag)
{
  m_positionFlag = positionFlag;
}
void
RudpHeader::SetTypeBits (uint8_t typeBits)
{
  m_typeBits = typeBits;
}
void
RudpHeader::SetInorderFlag (bool inorderFlag)
{
  m_inorderFlag = inorderFlag;
}
void
RudpHeader::SetSequenceNumber (uint32_t sequenceNumber)
{
  m_sequenceNumber = (sequenceNumber & 0x7fffffff);
}
void
RudpHeader::SetMessageNumber (uint32_t messageNumber)
{
  m_messageNumber = (messageNumber & 0x1ffffffff);
}
uint16_t 
RudpHeader::GetSourcePort (void) const
{
  return m_sourcePort;
}
uint16_t 
RudpHeader::GetDestinationPort (void) const
{
  return m_destinationPort;
}
bool
RudpHeader::GetControlFlag (void) const
{
  return m_controlFlag;
}
uint8_t
RudpHeader::GetPositionFlag (void) const
{
  return m_positionFlag;
}
uint8_t
RudpHeader::GetTypeBits (void) const
{
  return m_typeBits;
}
bool
RudpHeader::GetInorderFlag (void) const
{
  return m_inorderFlag;
}
uint32_t
RudpHeader::GetSequenceNumber (void) const
{
  return m_sequenceNumber;
}
uint32_t
RudpHeader::GetMessageNumber (void) const
{
  return m_messageNumber;
}

void
RudpHeader::ForcePayloadSize (uint16_t payloadSize)
{
  m_payloadSize = payloadSize;
}

TypeId 
RudpHeader::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RudpHeader")
    .SetParent<Header> ()
    .SetGroupName ("Internet")
    .AddConstructor<RudpHeader> ()
  ;
  return tid;
}

TypeId 
RudpHeader::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

void 
RudpHeader::Print (std::ostream &os) const
{
  os << "length: " << m_payloadSize + GetSerializedSize ()
     << ", " 
     << m_sourcePort << " > " << m_destinationPort
     << ", "
     << " S.No.: " << m_sequenceNumber
     << ", "
     << " M.No.: " << m_messageNumber
     << ", "
     << " control flag: " << m_controlFlag
     << ", "
     << " inorder flag: " << m_inorderFlag
     << ", "
     << " type bits: " << m_typeBits
     << ", "
     << " postion flag: " << m_positionFlag
  ;
}

uint32_t 
RudpHeader::GetSerializedSize (void) const
{
  return 16;
}

void
RudpHeader::Serialize (Buffer::Iterator start) const
{
  Buffer::Iterator i = start;

  i.WriteHtonU16 (m_sourcePort);
  i.WriteHtonU16 (m_destinationPort);

  if (m_payloadSize == 0)
    {
      i.WriteHtonU16 (start.GetSize ());
    }
  else
    {
      i.WriteHtonU16 (m_payloadSize);
    }
  
  i.WriteHtonU32 (((m_controlFlag<<31)|(m_sequenceNumber)));

  if(m_controlFlag)
  {
    i.WriteHtonU32 (((m_typeBits<<29)|(m_messageNumber)));
  }
  else
  {
    i.WriteHtonU32 (((m_positionFlag<<30)|(m_inorderFlag<<29)|(m_messageNumber)))
  }

}

uint32_t
RudpHeader::Deserialize (Buffer::Iterator start)
{
  Buffer::Iterator i = start;
  m_sourcePort = i.ReadNtohU16 ();
  m_destinationPort = i.ReadNtohU16 ();
  m_payloadSize = i.ReadNtohU16 () - GetSerializedSize ();
  rudpSequenceNumber = i.ReadNtohU32 ();
  rudpMessageNumber = i.ReadNtohU32 ();
  m_controlFlag = (rudpSequenceNumber & 0x80000000);
  m_sequenceNumber = (rudpSequenceNumber & 0x7fffffff);
  m_messageNumber = (rudpMessageNumber & 0x1fffffff);
  if(m_controlFlag)
  {
    m_typeBits = (rudpMessageNumber>>29);
  }
  else
  {
    m_positionFlag = (rudpMessageNumber>>30);
    m_inorderFlag = ((rudpMessageNumber>>29) & 1);
  }
  return GetSerializedSize ();
}

} // namespace ns3
