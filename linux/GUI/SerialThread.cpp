#include "SerialThread.h"

SerialThread::SerialThread(size_t noTaxels)
   : input(noTaxels)
{
}

bool SerialThread::connect(const QString &sDevice)
{
	try {
		input.connect(sDevice.toStdString());
		start();
	} catch (const std::exception &e) {
		emit statusMessage(e.what(), 2000);
	}
	return input.isConnected();
}

bool SerialThread::disconnect()
{
	if (!input.isConnected()) return false;
	input.disconnect();
	wait(); // for thread to finish
	return true;
}

void SerialThread::run()
{
	while (input.isConnected()) {
		try {
			updateFunc(input.readFrame());
		} catch (const std::exception &e) {
			if (input.isConnected())
				emit statusMessage(e.what(), 2000);
		}
	}
}
