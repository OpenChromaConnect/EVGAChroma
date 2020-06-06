
#include <stdint.h>
#include <stdio.h>
#include <Windows.h>
#include "OlsApi.h"
#include "OlsDef.h"
#include "Acx30.h"

WORD g_SMBusBase;

bool InitSMBus()
{
	if ((ReadPciConfigDword(PciBusDevFunc(0, 31, 4), 8) & 0xFFFFFF00) != 0xC050000)
	{
		printf("Invalid SMBus class!");
		return false;
	}

	auto Base = ReadPciConfigDword(PciBusDevFunc(0, 31, 4), 0x20);

	if (Base & 1)
	{
		g_SMBusBase = Base & 0xFFFC;
		return true;
	}

	return false;
}

bool ReadLedRegister(uint8_t addr, uint8_t *value)
{
	uint8_t read;
	for (int i = 0; i < 10; i++)
	{
		do
		{
			WriteIoPortByte(g_SMBusBase, -1);
			read = ReadIoPortByte(g_SMBusBase);
		} while (read & 0x9F);

		WriteIoPortByte(g_SMBusBase + 4, LED_UNK | 1);
		WriteIoPortByte(g_SMBusBase + 3, addr);
		WriteIoPortByte(g_SMBusBase + 2, 0x48);

		for (int j = 0; j < 100; j++)
		{
			Sleep(1);
			read = ReadIoPortByte(g_SMBusBase);
			if (!(read & 4) || read & 2)
				break;
		}

		if (~(read >> 2) & 1)
		{
			*value = ReadIoPortByte(g_SMBusBase + 5);
			return true;
		}
		Sleep(50);
	}

	return false;
}

bool WriteLedRegister(uint8_t addr, uint8_t value)
{
	uint8_t read;
	for (int i = 0; i < 10; i++)
	{
		WriteIoPortByte(g_SMBusBase, 0xFE);
		WriteIoPortByte(g_SMBusBase + 4, LED_UNK & 0xFE);
		WriteIoPortByte(g_SMBusBase + 3, addr);
		WriteIoPortByte(g_SMBusBase + 5, value);
		WriteIoPortByte(g_SMBusBase + 2, 0x48);

		for (int j = 0; j < 100; j++)
		{
			Sleep(1);
			read = ReadIoPortByte(g_SMBusBase);
			if (!(read & 1))
				break;
		}

		if (~(read >> 2) & 1)
			return true;

		Sleep(50);
	}

	return false;
}

bool ledInit()
{
	uint8_t reg1 = 0;
	if (!ReadLedRegister(LED_1, &reg1))
	{
		printf("Failed to read LED 1\n");
		return false;
	}

	uint8_t ptype = 0;
	if (!ReadLedRegister(LED_PTYPE, &ptype))
	{
		printf("Failed to read LED PType\n");
		return false;
	}

	return true;
}

bool SetLedMode(uint8_t mode)
{
	bool ret = false;
	uint8_t temp;

	WriteLedRegister(LED_CONTROL, 0xE5);
	WriteLedRegister(LED_CONTROL, 0xE9);
	ReadLedRegister(LED_CONTROL, &temp);

	if (WriteLedRegister(LED_MODE, mode))
		ret = true;

	WriteLedRegister(LED_CONTROL, 0xE0);
	ReadLedRegister(LED_CONTROL, &temp);
	return ret;
}

bool SetLedColor(uint8_t red, uint8_t green, uint8_t blue)
{
	bool ret = false;
	uint8_t temp;

	WriteLedRegister(LED_CONTROL, 0xE5);
	WriteLedRegister(LED_CONTROL, 0xE9);
	ReadLedRegister(LED_CONTROL, &temp);

	if (WriteLedRegister(LED_RED, red))
		ret |= true;

	if (WriteLedRegister(LED_GREEN, green))
		ret |= true;

	if (WriteLedRegister(LED_BLUE, blue))
		ret |= true;

	WriteLedRegister(LED_CONTROL, 0xE0);
	ReadLedRegister(LED_CONTROL, &temp);
	return !ret;
}
