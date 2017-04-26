/*
 * VB-ANT - VirtualBox - Advanced Network Tool
 * Copyright (C) 2016  Dario Messina
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

#ifndef EXAMSESSION_H
#define EXAMSESSION_H

#include <QString>
#include <QDateTime>
#include <mutex>
#include <memory>
#include <iostream>

class ExamSession
{
	private:
		static std::shared_ptr<ExamSession> _instance;
		static std::once_flag only_one;

		ExamSession(QString nome, QString cognome, QString matricola)
		: nome(nome), cognome(cognome), matricola(matricola)
		{
			std::cout << "ExamSession::Singleton()" << std::endl;
		}

		ExamSession(const ExamSession& rs)
		{
			_instance = rs._instance;
		}

		ExamSession& operator = (const ExamSession& rs)
		{
			if (this != &rs)
			{
				_instance = rs._instance;
			}

			return *this;
		}
		QString nome, cognome, matricola;

	public:
		~ExamSession()
		{
			std::cout << "Singleton::~Singleton" << std::endl;
		};

		static ExamSession &set(QString nome, QString cognome, QString matricola)
		{
			std::call_once(ExamSession::only_one,
					[](QString nome, QString cognome, QString matricola)
					{
						ExamSession::_instance.reset(new ExamSession(nome, cognome, matricola));
						std::cout << "ExamSession::create_singleton_(" << nome.toStdString() << ", " << cognome.toStdString() << ", " << matricola.toStdString() << ")" << std::endl;
					}, nome, cognome, matricola);
			return *ExamSession::_instance;
		};

		static ExamSession &get() { return *ExamSession::_instance; };
		bool operator==(std::nullptr_t ptr) { return ExamSession::_instance == nullptr; };

		bool saveTo(QString filename);
};
std::once_flag ExamSession::only_one;
std::shared_ptr<ExamSession> ExamSession::_instance = nullptr;

#endif //EXAMSESSION_H
