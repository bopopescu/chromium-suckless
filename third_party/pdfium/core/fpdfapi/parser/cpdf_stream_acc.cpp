// Copyright 2016 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_stream_acc.h"

#include "core/fpdfapi/parser/fpdf_parser_decode.h"

CPDF_StreamAcc::CPDF_StreamAcc()
    : m_pData(nullptr),
      m_dwSize(0),
      m_bNewBuf(FALSE),
      m_pImageParam(nullptr),
      m_pStream(nullptr),
      m_pSrcData(nullptr) {}

void CPDF_StreamAcc::LoadAllData(const CPDF_Stream* pStream,
                                 FX_BOOL bRawAccess,
                                 uint32_t estimated_size,
                                 FX_BOOL bImageAcc) {
  if (!pStream)
    return;

  m_pStream = pStream;
  if (pStream->IsMemoryBased() &&
      (!pStream->GetDict()->KeyExist("Filter") || bRawAccess)) {
    m_dwSize = pStream->GetRawSize();
    m_pData = pStream->GetRawData();
    return;
  }
  uint8_t* pSrcData;
  uint32_t dwSrcSize = pStream->GetRawSize();
  if (dwSrcSize == 0)
    return;

  if (!pStream->IsMemoryBased()) {
    pSrcData = m_pSrcData = FX_Alloc(uint8_t, dwSrcSize);
    if (!pStream->ReadRawData(0, pSrcData, dwSrcSize))
      return;
  } else {
    pSrcData = pStream->GetRawData();
  }
  if (!pStream->GetDict()->KeyExist("Filter") || bRawAccess) {
    m_pData = pSrcData;
    m_dwSize = dwSrcSize;
  } else {
    FX_BOOL bRet = PDF_DataDecode(pSrcData, dwSrcSize, m_pStream->GetDict(),
                                  m_pData, m_dwSize, m_ImageDecoder,
                                  m_pImageParam, estimated_size, bImageAcc);
    if (!bRet) {
      m_pData = pSrcData;
      m_dwSize = dwSrcSize;
    }
  }
  if (pSrcData != pStream->GetRawData() && pSrcData != m_pData)
    FX_Free(pSrcData);
  m_pSrcData = nullptr;
  m_bNewBuf = m_pData != pStream->GetRawData();
}

CPDF_StreamAcc::~CPDF_StreamAcc() {
  if (m_bNewBuf)
    FX_Free(m_pData);
  FX_Free(m_pSrcData);
}

const uint8_t* CPDF_StreamAcc::GetData() const {
  if (m_bNewBuf)
    return m_pData;
  return m_pStream ? m_pStream->GetRawData() : nullptr;
}

uint32_t CPDF_StreamAcc::GetSize() const {
  if (m_bNewBuf)
    return m_dwSize;
  return m_pStream ? m_pStream->GetRawSize() : 0;
}

uint8_t* CPDF_StreamAcc::DetachData() {
  if (m_bNewBuf) {
    uint8_t* p = m_pData;
    m_pData = nullptr;
    m_dwSize = 0;
    return p;
  }
  uint8_t* p = FX_Alloc(uint8_t, m_dwSize);
  FXSYS_memcpy(p, m_pData, m_dwSize);
  return p;
}
