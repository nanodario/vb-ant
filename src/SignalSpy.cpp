/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2015 - 2017  Dario Messina
 *
 * This file is part of VB-ANT
 *
 * VB-ANT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * VB-ANT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "SignalSpy.h"
#include <iostream>
#include <unistd.h>
#include <typeinfo>

static void *spyThread(void *_thread_data)
{
	thread_data_t *thread_data = (thread_data_t *)_thread_data;
	void *obj = thread_data->obj;

	QString signalName = thread_data->signal;
	signalName = signalName.mid(0, signalName.indexOf("("));

	while(1)
	{
		if(thread_data->spy == NULL)
			break;

		if(thread_data->spy->count())
		{
			QList<QVariant> arg = thread_data->spy->takeFirst();
			std::cout << "[" << __PRETTY_FUNCTION__ << "] " << typeid(obj).name() << " emitted signal: " << signalName.toStdString() << "(";
			for(int i = 0; i < arg.length(); i++)
			{
				std::cout << "(" << arg.at(i).typeName() << ") " << arg.at(i).toString().toStdString();
				if(i < arg.length() - 1)
					std::cout << ", ";
			}
			std::cout << ")" << std::endl;
		}
		usleep(250000);
	}
	
	std::cout << "QSignalSpy object destroyed, stopping spying ;)" << std::endl;
	
	int retval = 0;
	pthread_exit(&retval);
}

SignalSpy::SignalSpy(QObject *obj, const char *signal)
: spy(new QSignalSpy(obj, signal)), thread_data((thread_data_t *)malloc(sizeof(thread_data_t)))
{
	std::cout << "[" << __PRETTY_FUNCTION__ << "] " << "SignalSpy initialized. Obj: \"" << obj->objectName().toStdString() << "\", signal: \"" << signal << "\""<< std::endl;
	thread_data->obj = obj;
	thread_data->signal = signal;
	thread_data->spy = spy;

	launchSpyThread(thread_data);
}

SignalSpy::~SignalSpy()
{
	delete spy;
	int *retval;
	pthread_join(thread, (void **)&retval);
	free(thread_data);
}

void SignalSpy::launchSpyThread(thread_data_t *thread_data)
{
	pthread_create(&thread, NULL, spyThread, thread_data);
}
