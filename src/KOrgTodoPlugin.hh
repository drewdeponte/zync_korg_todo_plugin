/*
 * Copyright 2004, 2005 Andrew De Ponte
 * 
 * This file is part of zsrep.
 * 
 * zsrep is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * any later version.
 * 
 * zsrep is distributed in the hopes that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with zsrep; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/**
 * @file KOrgTodoPlugin.hh
 * @brief A specifications file for an object that is a TodoPlugin.
 * @author Andrew De Ponte
 *
 * A specifications file for an object that is a TodoPlugin for the
 * ZaurusSyncer application. This plugin is designed to allow the ZaurusSyncer 
 * application to sync with the KOrganizer's Todo list.
 */

#ifndef KORGTODOPLUGIN_H
#define KORGTODOPLUGIN_H

#define TODO_PLUGIN_VERSION "1.0.2"

// Plugin Includes
#include <zync/TodoPluginType.hh>

// KOrganizer Includes
#include <qstring.h>
#include <qdatetime.h>

#include <kinstance.h>
#include <kaboutdata.h>
#include <kconfig.h>
#include <libkcal/todo.h>
#include <libkcal/calendar.h>
#include <libkcal/calendarresources.h>

#include <libkcal/calendarlocal.h>

#include <iostream>
#include <fstream>

// Config File Includes
#include <confmgr/ConfigManagerType.h>
#include <stdlib.h>
#include <stdio.h>

/**
 * @class KOrgTodoPlugin
 * @brief A type existing as a plugin to sync with KOrganizer's Todo list.
 *
 * The KOrgTodoPlugin class is the implementation of a plugin which allows for
 * the ZaurusSyncer application to synchronize its Todo list with the Todo
 * list stored in KOrganizer.
 */
class KOrgTodoPlugin : public TodoPluginType {
public:
    KOrgTodoPlugin(void);

    int Initialize(void);
    int CleanUp(void);

    TodoItemType::List GetAllTodoItems(void);
    TodoItemType::List GetNewTodoItems(time_t lastTimeSynced);
    TodoItemType::List GetModTodoItems(time_t lastTimeSynced);
    SyncIDListType GetDelTodoItemIDs(time_t lastTimeSynced);
    int AddTodoItems(TodoItemType::List todoItems);
    int ModTodoItems(TodoItemType::List todoItems);
    int DelTodoItems(SyncIDListType todoItemIDs);
    int MapItemIDs(TodoItemType::List todoItems);

    std::string GetPluginDescription(void) const;
    std::string GetPluginName(void) const;
    std::string GetPluginAuthor(void) const;
    std::string GetPluginVersion(void) const;
private:
    int GetAllTodoSyncItems(time_t lastTimeSynced,
			    TodoItemType::List &newItemList,
			    TodoItemType::List &modItemList,
			    SyncIDListType &delItemIdList);
    int SaveSyncIDLog(void);
    TodoItemType ConvKCalTodo(KCal::Todo *pKcalTodo);
    KCal::Todo *ConvTodoItemType(TodoItemType *pTodoItem);
    void UpdateKCalTodoItem(KCal::Todo *pKCalTodo, TodoItemType todoItem);
    time_t ConvQDateTime(QDateTime dateTime);

    KAboutData *pKAboutData;
    KInstance *pKInstance;
//    KCal::CalendarResources *pCalRes;
    KCal::CalendarLocal *pCal;

    
    /*
    KCal::CalendarLocal calendar;
    */
    QString qCalPath;
    bool openedCalFlag;
    bool openedConfFlag;

    std::string homeDir;


    bool obtainedSyncLists;
    TodoItemType::List newTodoItemList;
    TodoItemType::List modTodoItemList;
    SyncIDListType delTodoItemIdList;

    
};

#endif
