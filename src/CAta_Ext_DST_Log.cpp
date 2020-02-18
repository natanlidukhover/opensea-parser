//
// CAta_Ext_DST_Log.cpp
//
// Do NOT modify or remove this copyright and license
//
// Copyright (c) 2014 - 2020 Seagate Technology LLC and/or its Affiliates, All Rights Reserved
//
// This software is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.
//
// ******************************************************************************************

#include "CAta_Ext_DST_Log.h"

using namespace opensea_parser;
//-----------------------------------------------------------------------------
//
//! \fn   CAta_Ext_DST_Log::CAta_Ext_DST_Log(
//
//! \brief
//!   Description:  constructor of the Ext DST Log Class
//
//  Entry:
//
//  Exit:DST_Enum
//
//---------------------------------------------------------------------------
CAta_Ext_DST_Log::CAta_Ext_DST_Log(const std::string &fileName, JSONNODE *masterData)
    :m_name("Ext DST Log")                                         //!< name of the class
    , m_status(IN_PROGRESS)
{
	CLog *cCLog;
	cCLog = new CLog(fileName);
	if (cCLog->get_Log_Status() == SUCCESS)
	{
		if (cCLog->get_Buffer() != NULL)
		{
			m_logSize = cCLog->get_Size();
			pData = new uint8_t[m_logSize];								// new a buffer to the point				
#ifndef _WIN64 
			memcpy(pData, cCLog->get_Buffer(), m_logSize);
#else
			memcpy_s(pData, m_logSize, cCLog->get_Buffer(), m_logSize);// copy the buffer data to the class member pBuf
#endif
			sLogPageStruct *idCheck;
			idCheck = (sLogPageStruct *)&pData[0];
			byte_Swap_16(&idCheck->pageLength);
			if (IsScsiLogPage(idCheck->pageLength, idCheck->pageCode) == false)
			{
				byte_Swap_16(&idCheck->pageLength);  // now that we know it's not scsi we need to flip the bytes back
				m_status = parse_Ext_Self_Test_Log( masterData);
			}
            delete [] pData;
        }
    }
    else
    {
        m_status = IN_PROGRESS;
        m_logSize = 0;
    }
	delete (cCLog);
}
//-----------------------------------------------------------------------------
//
//! \fn   CAta_Ext_DST_Log::CAta_Ext_DST_Log(
//
//! \brief
//!   Description:  constructor of the Ext DST Log Class
//
//  Entry:
//
//  Exit:DST_Enum
//
//---------------------------------------------------------------------------
CAta_Ext_DST_Log::CAta_Ext_DST_Log(uint8_t *pBufferData, JSONNODE *masterData)
    :m_name("Ext DST Log")                                         //!< name of the class
    , m_status(IN_PROGRESS)
{
	pData = pBufferData;
    m_logSize = 0;
    m_status = parse_Ext_Self_Test_Log( masterData);
    pData = NULL;
}
//-----------------------------------------------------------------------------
//
//! \fn   CAta_Ext_DST_Log::~CAta_Ext_DST_Log(
//
//! \brief
//!   Description:  Deconstructor of the Ext DST Log Class
//
//  Entry:
//
//  Exit:DST_Enum
//
//---------------------------------------------------------------------------
CAta_Ext_DST_Log::~CAta_Ext_DST_Log()
{

}
//-----------------------------------------------------------------------------
//
//! \fn Get_Status_Meaning
//
//! \brief
//!   Description: fill in a string of the meaning of the status data
//
//  Entry:
//! \param meaning - string to fill in the meaing
//! \param status - the status to fill in the meaing 
//
//  Exit:
//!   \return eReturnValues success
//
//---------------------------------------------------------------------------
void CAta_Ext_DST_Log::Get_Status_Meaning(std::string &meaning,uint8_t status)
{
    meaning.resize(BASIC);
    if (status == 0x00)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Self Test has completed with no Error  or No Self test has been run");
    }
    else if (status == 0x01)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Was Aborted by the host");
    }
    else if (status == 0x02)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Was interepted by the host with a hard reset of a soft reset");
    }
    else if (status == 0x03)
    {
        snprintf((char*)meaning.c_str(),BASIC,"unknown error and Self Test was unable to complete");
    }
    else if (status == 0x04)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Completed and has faild and the element is unknown");
    }
    else if (status == 0x05)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Completed With an electrical element failing");
    }
    else if (status == 0x06)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Completed having a servo element failure");
    }
    else if (status == 0x07)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Completed having a read element failure");
    }
    else if (status == 0x08)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Completed having handling damage");
    }
    else if (status == 0x09)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Completed having suspected handling damage");
    }
    else if (status == 0x15)
    {
        snprintf((char*)meaning.c_str(),BASIC,"Self Test still in Progress");
    }
    else
    {
        snprintf((char*)meaning.c_str(),BASIC,"Reserved");
    }
}
//-----------------------------------------------------------------------------
//
//! \fn CAta_Ext_DST_Log::parse_Ext_Self_Test_Log()
//
//! \brief
//!   Description: parse the ext self test dst log number 7
//
//  Entry:
//! \param pData  pointer to the buffer data
//
//  Exit:
//!   \return eReturnValues success
//
//---------------------------------------------------------------------------
eReturnValues CAta_Ext_DST_Log::parse_Ext_Self_Test_Log( JSONNODE *masterData)
{
    std::string myStr = "Start of Ext DST";
    JSONNODE *DstJson = json_new(JSON_NODE);
    uint16_t DSTIndex = 0;
    uint8_t StatusByte = 0;
    uint16_t timeStamp = 0;
    uint16_t compTime = 0;
    int8_t checkPointByte = 0;
    uint64_t LBA = 0;

    json_set_name(DstJson, "DST Log");
	uint16_t index = ((uint16_t)pData[3] << 8) | ((uint16_t)pData[2] << 0);
	json_push_back(DstJson, json_new_i("Self Test Index", static_cast<uint32_t>(index)));
    DSTIndex += 4;
    for (int i = 1; i <= 19; i++)
    {

        
        StatusByte = pData[DSTIndex + 1];
        timeStamp = ((uint16_t)pData[DSTIndex + 3] << 8) | ((uint16_t)pData[DSTIndex + 2] << 0);
        compTime = ((uint16_t)pData[DSTIndex + 13] << 8) | ((uint16_t)pData[DSTIndex + 12] << 0);
        checkPointByte = pData[DSTIndex + 4];
        LBA = ((uint64_t)pData[DSTIndex + 10] << 40) |
            ((uint64_t)pData[DSTIndex + 9] << 32) |
            ((uint64_t)pData[DSTIndex + 8] << 24) |
            ((uint64_t)pData[DSTIndex + 7] << 16) |
            ((uint64_t)pData[DSTIndex + 6] << 8) |
            ((uint64_t)pData[DSTIndex + 5] << 0);
        
        JSONNODE *runInfo = json_new(JSON_NODE);
        snprintf((char*)myStr.c_str(), BASIC, "Run %3d ", i);
        json_set_name(runInfo, (char*)myStr.c_str());
        snprintf((char*)myStr.c_str(), BASIC, "%u", timeStamp);
        json_push_back(runInfo, json_new_a("Timestamp", (char*)myStr.c_str()));
        snprintf((char*)myStr.c_str(), BASIC, "0x%02x", int(StatusByte));
        json_push_back(runInfo, json_new_a("Status Byte", (char*)myStr.c_str()));
        Get_Status_Meaning(myStr,StatusByte);
        json_push_back(runInfo, json_new_a("Status Meaning",(char*)myStr.c_str()));

        snprintf((char*)myStr.c_str(), BASIC, "0x%02x", checkPointByte);
        json_push_back(runInfo, json_new_a("CheckPoint Byte", (char*)myStr.c_str()));
        snprintf((char*)myStr.c_str(), BASIC, "%u", compTime);
        json_push_back(runInfo, json_new_a("Completion Time", (char*)myStr.c_str()));
        set_json_64bit(runInfo, "LBA", LBA, false);

        DSTIndex += 26;
        json_push_back(DstJson, runInfo);
    }

    json_push_back(masterData, DstJson);

    return SUCCESS;

}


