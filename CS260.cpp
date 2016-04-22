#include "CS260.h"

#define ReplyBufferSize 16
#define IntFloatBufferSize 16
#define AlmostEqualDifference 0.2f

const char CS260::CarriageReturnCharacter = '\r';
const char CS260::EndOfLineCharacter = '\n';
const char CS260::SpaceCharacter = ' ';
const char CS260::GetCharacter = '?';
const char CS260::ClosedCharacter = 'C';
const char CS260::OpenedCharacter = 'O';
const char CS260::PeriodCharacter = '.';
const uint32_t CS260::TimeOut = 10000000;
const uint8_t CS260::MaxCommandRetries = 8;
const uint8_t CS260::MaxWavelengthSetRetires = 8;
const float CS260::WavelengthMin = 190;
const float CS260::WavelengthMax = 1000;
const CS260::CommandStringList CS260::CommandLibrary[] =
{
	{CommandsType::GoWave,CommandValueType::Float,"GOWAVE",6,true},
	{CommandsType::Calibrate,CommandValueType::Float,"CALIBRATE",9,false},
	{CommandsType::Grat,CommandValueType::Integer,"GRAT",4,true},
	{CommandsType::Shutter,CommandValueType::Integer,"SHUTTER",7,false},
	{CommandsType::Outport,CommandValueType::Integer,"OUTPORT",7,false},
	{CommandsType::Wave,CommandValueType::Float,"WAVE",4,false} //Need to poll wavelength for this must be false, or endless loop occurs.
};
CS260::CS260(HardwareSerial *serial)
{
	_HardwareSerial = serial;
	CurrentRecievingPart = RecievingPart::Echo;
	ReplyBuffer = new char[ReplyBufferSize]();
	IntFloatBuffer = new char[IntFloatBufferSize]();
	RecievedCallback = 0;
	ReplyBufferIndex = 0;
	CommandRetries = 0;
	TransmitTime = 0;
	CurrentWavelength = 0.0;
	WavelengthSetRetries = 0;
	PollingWavelength = false;
	Busy = false;
	ExpectReply = false;
}
CS260::~CS260()
{

}
void CS260::SetupForSending(bool ResetWavelengthSetRetries)
{
	Busy = true;
	CommandRetries = 0;
	if (ResetWavelengthSetRetries)
	{
		WavelengthSetRetries = 0;
	}
}
bool CS260::SendSetWavelength(float Wavelength)
{
	if (!Busy)
	{
		SetupForSending(false);
		Wavelength = constrain(Wavelength, WavelengthMin, WavelengthMax);
		CurrentWavelength = Wavelength;
		uint8_t Index = static_cast<uint8_t>(CommandsType::GoWave);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Set;
		CurrentCommand.Value.FloatValue = Wavelength;
		CurrentCommand.ValueType = CommandValueType::Float;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendGetWavelength()
{
	if (!Busy)
	{
		SetupForSending(false);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Wave);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Get;
		CurrentCommand.ValueType = CommandValueType::Float;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendCalibrate(float CalibrationAdjustment)
{
	if (!Busy)
	{
		SetupForSending(true);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Calibrate);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Set;
		CurrentCommand.Value.FloatValue = CalibrationAdjustment;
		CurrentCommand.ValueType = CommandValueType::Float;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendSetGrating(uint8_t GratingNumber)
{
	if (!Busy)
	{
		SetupForSending(true);
		GratingNumber = constrain(GratingNumber, 1, 3);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Grat);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Set;
		CurrentCommand.Value.IntegerValue = GratingNumber;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendGetGrating()
{
	if (!Busy)
	{
		SetupForSending(true);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Grat);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Get;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendSetShutter(bool Open)
{
	if (!Busy)
	{
		SetupForSending(true);
		uint8_t Value = 0;
		if (Open)
		{
			Value = 1;
		}
		else
		{
			Value = 0;
		}
		uint8_t Index = static_cast<uint8_t>(CommandsType::Shutter);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Set;
		CurrentCommand.Value.IntegerValue = Value;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendGetShutter()
{
	if (!Busy)
	{
		SetupForSending(true);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Shutter);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Get;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendSetOutport(uint8_t Outport)
{
	if (!Busy)
	{
		SetupForSending(true);
		Outport = constrain(Outport, 1, 2);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Outport);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Set;
		CurrentCommand.Value.IntegerValue = Outport;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SendGetOutport()
{
	if (!Busy)
	{
		SetupForSending(true);
		uint8_t Index = static_cast<uint8_t>(CommandsType::Outport);
		CurrentCommand.Command = CommandLibrary[Index].Command;
		CurrentCommand.NeedToPollWavelength = CommandLibrary[Index].NeedToPollWavelength;
		CurrentCommand.Type = TransmissionType::Get;
		CurrentCommand.ValueType = CommandValueType::Integer;
		return SendCurrentCommand();
	}
	else
	{
		return false;
	}
}
bool CS260::SetRecievedCallback(FinishedListener Finished)
{
	if (!Finished)
	{
		return false;
	}
	RecievedCallback = Finished;
	return true;
}
bool CS260::SendCurrentCommand()
{
	const uint8_t CommandIndex = static_cast<uint8_t>(CurrentCommand.Command);
	const uint8_t* StringToSend = reinterpret_cast<const uint8_t*>(CommandLibrary[CommandIndex].String);
	const uint8_t CharsToSend = CommandLibrary[CommandIndex].Count;
	_HardwareSerial->write(StringToSend, CharsToSend);
	//Serial.print("M:");
	//Serial.print(String((char*)StringToSend));
	if (CurrentCommand.Type == TransmissionType::Set)
	{
		_HardwareSerial->write(SpaceCharacter);
		//Serial.print(SpaceCharacter);
		if (CurrentCommand.Command == CommandsType::Shutter)
		{
			if (CurrentCommand.Value.IntegerValue == 1)
			{
				_HardwareSerial->write(OpenedCharacter);
			}
			else if (CurrentCommand.Value.IntegerValue == 0)
			{
				_HardwareSerial->write(ClosedCharacter);
			}
			else
			{
				return false;
			}
		}
		else
		{
			if (CurrentCommand.ValueType == CommandValueType::Integer)
			{
				int Count = IntToCharPointer(CurrentCommand.Value.IntegerValue, IntFloatBuffer, IntFloatBufferSize);
				_HardwareSerial->write(reinterpret_cast<const uint8_t*>(IntFloatBuffer),Count);
				//Serial.print(IntFloatBuffer);
			}
			else if (CurrentCommand.ValueType == CommandValueType::Float)
			{
				int Count = FloatToCharPointer(CurrentCommand.Value.FloatValue, IntFloatBuffer, IntFloatBufferSize);
				_HardwareSerial->write(reinterpret_cast<const uint8_t*>(IntFloatBuffer),Count);
				//Serial.print(IntFloatBuffer);
			}
			else
			{
				return false;
			}
		}
	}
	else if (CurrentCommand.Type == TransmissionType::Get)
	{
		_HardwareSerial->write(GetCharacter);
		//Serial.print(GetCharacter);
	}
	else if (CurrentCommand.Type == TransmissionType::None)
	{

	}
	else
	{
		return false;
	}
	_HardwareSerial->write(CarriageReturnCharacter);
	_HardwareSerial->write(EndOfLineCharacter);
	_HardwareSerial->flush();
	//Serial.print(EndOfLineCharacter);
	//Serial.print(CarriageReturnCharacter);
	TransmitTime = micros();
	ReplyBufferIndex = 0;
	CurrentRecievingPart = RecievingPart::Echo;
	CurrentReply.ValueType = ReplyValueType::None;
	ExpectReply = true;
	return true;
}
uint8_t CS260::IntToCharPointer(uint8_t Input, char* Buffer, size_t BufferSize)
{
	memset(Buffer,0,BufferSize);
	return sprintf(Buffer, "%u", Input);
}
uint8_t CS260::FloatToCharPointer(float Input, char* Buffer, size_t BufferSize)
{
	memset(Buffer,0,BufferSize);
	return sprintf(Buffer, "%1.3f", Input);
}
uint8_t CS260::CharPointerToInt(char* Buffer, size_t BufferSize)
{
	Buffer[BufferSize-1] = '\0';
	return (uint8_t) atoi(Buffer);
}
float CS260::CharPointerToFloat(char* Buffer, size_t BufferSize)
{
	Buffer[BufferSize-1] = '\0';
    float sign = 1.0;
    float value;
    while (isblank(*Buffer) )
    {
        Buffer++;
    }
    if (*Buffer == '-')
    {
        sign = -1.0;
        Buffer++;
    }
    for (value = 0.0; isdigit(*Buffer); Buffer++)
    {
        value = value * 10.0 + (*Buffer - '0');
    }
    if (*Buffer == '.')
    {
        double pow10 = 10.0;
        Buffer++;
        while (isdigit(*Buffer))
        {
            value += (*Buffer - '0') / pow10;
            pow10 *= 10.0;
            Buffer++;
        }
    }
    return sign * value;
}
void CS260::CheckSerial()
{
	//if (_HardwareSerial->available() > 0)
	//{
	//	Serial.print((char)_HardwareSerial->peek());
	//}
	if (ExpectReply)
	{
		uint32_t TimeDifference = micros()  - TransmitTime;
		if(_HardwareSerial->available() > 0)
		{
			char Character = (char)_HardwareSerial->read();
			switch (CurrentRecievingPart)
			{
				case RecievingPart::Echo:
					//Serial.print("$");
					//Serial.print(Character);
					ParseEcho(Character);
					break;
				case RecievingPart::Value:
					//Serial.print("^");
					//Serial.print(Character);
					ParseValue(Character);
					break;
				default:
					break;
			}
		}
		else if ( ( (!CheckWavelength) && ( TimeDifference > TimeOut ) ) || ( TimeDifference > (TimeOut*5) ) )
		{
			Serial.println("Timeout on monochromator.");
			CurrentReply.Command = CommandsType::Error;
			CurrentReply.Type = TransmissionType::Error;
			CurrentReply.ValueType = ReplyValueType::Error;
			ReplyBufferIndex = 0;
			CurrentRecievingPart = RecievingPart::Echo;
			CheckReply();
		}
	}
	else
	{
		if(_HardwareSerial->available() > 0)
		{
			_HardwareSerial->clear();
		}
	}
}
void CS260::ParseEcho(char Character)
{
	//Serial.print(Character);
	if (isalpha(Character))
	{
		ReplyBuffer[ReplyBufferIndex] = Character;
		if (ReplyBufferIndex < (ReplyBufferSize-2) )
		{
			ReplyBufferIndex++;
		}
	}
	else if ( (Character == GetCharacter) || (Character == SpaceCharacter) || (Character == EndOfLineCharacter) || (Character == CarriageReturnCharacter)  )
	{
		if (ReplyBufferIndex > 0)
		{
			CurrentRecievingPart = RecievingPart::Value;
			if ( Character == SpaceCharacter)
			{
				CurrentReply.Type = TransmissionType::Set;
			}
			else if ( Character == GetCharacter)
			{
				CurrentReply.Type = TransmissionType::Get;
			}
			else if ( !( (CurrentCommand.ValueType==CommandValueType::None) || (CurrentCommand.ValueType==CommandValueType::String) ) )
			{
				CurrentReply.Type = TransmissionType::Error;
				CurrentReply.ValueType = ReplyValueType::Error;
			}
			bool FoundCommand = false;
			ReplyBuffer[ReplyBufferIndex]='\0';
			//Serial.print("!B:");
			//Serial.print(ReplyBuffer);
			//Serial.print(":");
			for (uint8_t Index = 0; Index < static_cast<uint8_t>(CommandsType::Count); Index++)
			{
				if (strcmp(ReplyBuffer,CommandLibrary[Index].String)==0)
				{
					FoundCommand = true;
					CurrentReply.Command = CommandLibrary[Index].Command;
					switch (CommandLibrary[Index].ValueType)
					{
						case CommandValueType::Integer:
							CurrentReply.ValueType = ReplyValueType::Integer;
							//Serial.println("I!");
							break;
						case CommandValueType::Float:
							CurrentReply.ValueType = ReplyValueType::Float;
							//Serial.println("F!");
							break;
						case CommandValueType::None:
							CurrentReply.ValueType = ReplyValueType::None;
							//Serial.println("N!");
							break;
						case CommandValueType::String:
							CurrentReply.ValueType = ReplyValueType::String;
							//Serial.println("S!");
							break;
						default:
							CurrentReply.ValueType = ReplyValueType::Error;
							break;
					}
					break;
				}
			}
			if (!FoundCommand)
			{
				//Serial.print("!NF!");
				CurrentReply.Command = CommandsType::Error;
				CurrentReply.Type = TransmissionType::Error;
				CurrentReply.ValueType = ReplyValueType::Error;
				CurrentRecievingPart = RecievingPart::Echo;
				CheckReply();
			}
			ReplyBufferIndex = 0;
		}
	}
}
void CS260::ParseValue(char Character)
{
	//Serial.print(Character);
	if (isdigit(Character) || (Character == PeriodCharacter) )
	{
		ReplyBuffer[ReplyBufferIndex] = Character;
		if (ReplyBufferIndex < (ReplyBufferSize-1) )
		{
			ReplyBufferIndex++;
		}
	}
	else if ( (Character == CarriageReturnCharacter) || (Character == EndOfLineCharacter) || (Character == SpaceCharacter))
	{
		if (ReplyBufferIndex > 0)
		{
			if (CurrentReply.ValueType == ReplyValueType::Integer)
			{
				CurrentReply.Value.IntegerValue = CharPointerToInt(ReplyBuffer,ReplyBufferIndex);
				//Serial.print("!I:");
				//Serial.print(CurrentReply.Value.IntegerValue);
			}
			else if (CurrentReply.ValueType == ReplyValueType::Float)
			{
				CurrentReply.Value.FloatValue = CharPointerToFloat(ReplyBuffer,ReplyBufferIndex);
				//Serial.print("!F:");
				//Serial.print(CurrentReply.Value.FloatValue);
			}
			ReplyBufferIndex = 0;
			CurrentRecievingPart = RecievingPart::Echo;
			CheckReply();
		}
	}
	else if ( ( (Character == ClosedCharacter)||(Character == OpenedCharacter) ) && (CurrentReply.Command == CommandsType::Shutter) )
	{
		if (Character == ClosedCharacter)
		{
			CurrentReply.Value.IntegerValue = 0;
		}
		else
		{
			CurrentReply.Value.IntegerValue = 1;
		}
		ReplyBufferIndex = 0;
		CurrentRecievingPart = RecievingPart::Echo;
		CheckReply();
	}
	else
	{
		CurrentReply.Command = CommandsType::Error;
		CurrentReply.Type = TransmissionType::Error;
		CurrentReply.ValueType = ReplyValueType::Error;
		CurrentRecievingPart = RecievingPart::Echo;
		CheckReply();
	}
}
float CS260::GetCurrentWavelength()
{
	return CurrentWavelength;
}
void CS260::CheckReply()
{
	ExpectReply = false;
	//Serial.print("!C:");
	if (CurrentReply.Command == CommandsType::Error)
	{
		//Serial.println("E!");
		if (CommandRetries < MaxCommandRetries)
		{
			CommandRetries++;
			SendCurrentCommand();
		}
		else
		{
			CommandRetries = 0;
			Serial.println("Monochromator transmission failed.");
		}
	}
	else if (CurrentCommand.NeedToPollWavelength && (CurrentReply.Command != CommandsType::Wave))
	{
		//Serial.println("P!");
		CurrentCommand.NeedToPollWavelength = false;
		CheckWavelength = true;
		Busy = false;
		SendGetWavelength();
	}
	else
	{
		if ( CurrentReply.Command == CommandsType::Wave )
		{
			if (CheckWavelength)
			{
				if (AboutEqual(CurrentWavelength, CurrentReply.Value.FloatValue))
				{
					CheckWavelength = false;
					CurrentWavelength = CurrentReply.Value.FloatValue;
				}
				else
				{
					Busy = false;
					SendSetWavelength(CurrentWavelength);
					if (WavelengthSetRetries > MaxWavelengthSetRetires)
					{
						Serial.println("Monochromator wavelength reset failed.");
						WavelengthSetRetries = 0;
						CheckWavelength = false;
					}
					else
					{
						WavelengthSetRetries++;
					}
					return;
				}
			}
			else
			{
				CurrentWavelength = CurrentReply.Value.FloatValue;
			}
		}
		Busy = false;
		CommandRetries = 0;
		if (RecievedCallback)
		{
			RecievedCallback();
		}
	}
}
bool CS260::AboutEqual(float x, float y)
{
	if ( (x > y ? x - y : y - x) > AlmostEqualDifference)
	{
		return false;
	}
	else
	{
		return true;
	}
}