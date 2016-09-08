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
 * Author: 
 */

#ifndef RUDP_HEADER_H
#define RUDP_HEADER_H

#include <stdint.h>
#include <string>
#include "ns3/header.h"
#include "ns3/ipv4-address.h"
#include "ns3/ipv6-address.h"

namespace ns3 {
/**
 * \ingroup udp
 * \brief Packet header for UDP packets
 *
 * This class has fields corresponding to those in a network UDP header
 * (port numbers, payload size, checksum) as well as methods for serialization
 * to and deserialization from a byte buffer.
 */
class RudpHeader : public Header 
{
public:

  /**
   * \brief Constructor
   *
   * Creates a null header
   */
  RudpHeader ();
  ~RudpHeader ();

  /**
   * \param port the destination port for this UdpHeader
   */
  void SetDestinationPort (uint16_t port);
  /**
   * \param port The source port for this UdpHeader
   */
  void SetSourcePort (uint16_t port);
  /**
  * \param controlBit Control bit 1 if control packet, 0 if data packet
  */
  void SetControlFlag (bool controlBit);
  /**
  * \param pfBit Position flag for a data packet
  */
  void SetPositionFlag (uint8_t positionFlag);
  /**
  * \/param typeBit Type bits for a controle packet
  */
  void SetTypeBits (uint8_t typeBits);
  /*
  * \param inorderBit True if packets should be sent in order
  */
  void SetInorderFlag (bool inorderBit);
  /**
  * \param sequenceNumber The sequence number for the payload
  */
  void SetSequenceNumber (uint32_t sequenceNumber);
  /**
  * \param messageNumber The message number for the payload if the payload is fragmented
  */
  void SetMessageNumber (uint32_t messageNumber);
  /**
   * \return The source port for this UdpHeader
   */
  uint16_t GetSourcePort (void) const;
  /**
   * \return the destination port for this UdpHeader
   */
  uint16_t GetDestinationPort (void) const;
  /**
  * \return true if the packet is a control packet, else return false if data packet
  */
  bool GetControlFlag (void) const;
  /**
  * \return positionFlag Position flag for a data packet
  */
  uint8_t GetPositionFlag (void) const;
  /**
  * \return typeBit Type bits for a controle packet
  */
  uint8_t GetTypeBits (void) const;
  /*
  * \return inorderBit True if packets should be sent in order
  */
  bool GetInorderFlag (void) const;
  /**
  * \return the sequence number of the packet
  */
  uint32_t GetSequenceNumber (void) const;
  /**
  * \return the message number of the packet if the payload is fragmented
  */
  uint32_t GetMessageNumber (void) const;

  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual void Print (std::ostream &os) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (Buffer::Iterator start) const;
  virtual uint32_t Deserialize (Buffer::Iterator start);

  /**
   * \brief Force the UDP payload length to a given value.
   *
   * This might be useful when forging a packet for test
   * purposes.
   *
   * \param payloadSize the payload length to use.
   */
  void ForcePayloadSize (uint16_t payloadSize);

private:
  uint16_t m_sourcePort;      //!< Source port
  uint16_t m_destinationPort; //!< Destination port
  uint16_t m_payloadSize;     //!< Payload size

  uint32_t m_sequenceNumber;
  uint32_t m_messageNumber;
  uint8_t m_typeBits;
  uint8_t m_postionFlag;
  bool m_inOrderFlag;
  bool m_controlFlag;
   

  Address m_source;           //!< Source IP address
  Address m_destination;      //!< Destination IP address
  uint8_t m_protocol;         //!< Protocol number
};

} // namespace ns3

#endif /* UDP_HEADER */
