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
 * @file KOrgTodoPlugin.cc
 * @brief An implementation file for an object that is a TodoPlugin.
 * @author Andrew De Ponte
 *
 * An implementation file for an object that is a TodoPlugin for the
 * ZaurusSyncer application. This plugin is designed to allow the ZaurusSyncer
 * application to sync with the KOrganizer's Todo list.
 */

#include "KOrgTodoPlugin.hh"

TodoPluginType *createTodoPlugin() {
    return (TodoPluginType *)new KOrgTodoPlugin;
}

void destroyTodoPlugin(TodoPluginType *pTodoPlugin) {
    delete pTodoPlugin;
}

/**
 * Construct a default KOrgTodoPlugin object.
 *
 * Construct a defualt KOrgTodoPlugin object with all the basic
 * initialization.
 */
KOrgTodoPlugin::KOrgTodoPlugin(void) {
    openedCalFlag = false;
    openedConfFlag = true;
    obtainedSyncLists = false;
}

/**
 * Initialize the KOrgTodoPlugin object.
 *
 * Initialize the KOrgTodoPlugin object by loading in its configuration from
 * the config file and preparing it for synchronization.
 * @return An integer representing success (zero) or failure (non-zero).
 * @retval 0 Successfully initialized the KOrgTodo plugin.
 * @retval 1 Failed to obtain the value of the HOME environment variable.
 * @retval 2 Failed to open the plugin's associated KOrg calendar file.
 */
int KOrgTodoPlugin::Initialize(void) {
    char *pEnvVarVal;
    std::string confPath;
    std::string calPath;
    std::string korgConfPath;
    int retval;
    char optVal[256];
    ConfigManagerType confManager;

    // Obtain the value of the HOME environment variable and build the path to
    // the config file so that it can be loaded.
    pEnvVarVal = getenv("HOME");
    if (!pEnvVarVal) {
	std::cout << "KOrgTodoPlugin: Error: Failed to obtain the HOME " \
	    "environment variable.\n";
	return 1;
    }

    homeDir.assign(pEnvVarVal);

    pKAboutData = new KAboutData("KOrgTodoPlugin",
				 "Zync KOrganizer Todo Plugin",
				 GetPluginVersion().c_str());
    if (!pKAboutData) {
	std::cout << "KOrgTodoPlugin::Initialize - ";
	std::cout << "Failed to allocate mem for KAboutData object.\n";
	return 1;
    }

    pKInstance = new KInstance(pKAboutData);
    if (!pKInstance) {
	std::cout << "KOrgTodoPlugin::Initialize - ";
	std::cout << "Failed to allocate mem for KInstance object.\n";
	delete pKAboutData;
	return 2;
    }

    confPath.assign(pEnvVarVal);
    confPath.append("/.KOrgTodoPlugin.conf");

    // Now I attempt to open and load the config file.
    retval = confManager.Open((char *)confPath.c_str());
    if (retval != 0) {
	if (retval == -1) {
	    std::cout << "KOrgTodoPlugin: Error: Failed to open " << confPath
		      << " for reading.\n";
	} else if (retval == -2) {
	    std::cout << "KOrgTodoPlugin: Error: Failed to find an equals" \
		" on atleast one non comment line in the config file. These" \
		" lines of the config file have been ignored and the config" \
		" file has been loaded. Fix your config file, it is probably" \
		" a typo.\n";
	} else {
	    std::cout << "KOrgTodoPlugin: Error: An unhandled error occured" \
		" while trying to open the config file.\n";
	}
	openedConfFlag = false;
    }

    // Here I attempt to load the path to the calendar file from the config.
    if (openedConfFlag) {
	retval = confManager.GetValue("korg_cal_path", optVal, 256);
	if (retval == 0) {
	    calPath.assign(optVal);
	} else {
	    calPath.assign(pEnvVarVal);
	    calPath.append("/.kde/share/apps/korganizer/std.ics");
	    std::cout << "KOrgTodoPlugin: Warning: Failed to find an item" \
		" with the title " \
		"(korg_cal_path) in the config file (" << confPath << ")." \
		" Using the default value (" << calPath << ").\n";
	}
    } else {
	calPath.assign(pEnvVarVal);
	calPath.append("/.kde/share/apps/korganizer/std.ics");
	std::cout << "KOrgTodoPlugin: Warning: The above described error " \
	    " states that there was a failure in opening the config" \
	    " file (" << confPath << "). Due, to this failure the KOrganizer" \
	    " Config path is now assumed to be (" << calPath << ").\n";
    }

    // Here I attempt to load the path to the korganizer config.
    if (openedConfFlag) {
	retval = confManager.GetValue("korg_conf_path", optVal, 256);
	if (retval == 0) {
	    korgConfPath.assign(optVal);
	} else {
	    korgConfPath.assign(pEnvVarVal);
	    korgConfPath.append("/.kde/share/config/korganizer/korganizerrc");
	    std::cout << "KOrgTodoPlugin: Warning: Failed to find an item" \
		" with the title " \
		"(korg_conf_path) in the config file (" << confPath << ")." \
		" Using the default value (" << korgConfPath << ").\n";
	}
    } else {
	korgConfPath.assign(pEnvVarVal);
	korgConfPath.append("/.kde/share/config/korganizer/korganizerrc");
	std::cout << "KOrgTodoPlugin: Warning: The above described error " \
	    " states that there was a failure in opening the config" \
	    " file (" << confPath << "). Due, to this failure the KOrganizer" \
	    " Calendar path is now assumed to be (" << korgConfPath << ").\n";
    }

    KConfig korgcfg(korgConfPath.c_str());
    korgcfg.setGroup("Time & Date");
    
    pCal = new KCal::CalendarLocal(korgcfg.readEntry("TimeZoneId"));
    if (!pCal) {
	std::cout << "KOrgTodoPlugin::Initialize - ";
	std::cout << "Failed to allocate mem for CalendarLocal object.\n";
	delete pKAboutData;
	return 3;
    }

    // Load the file located at calPath into the calendar object.
    qCalPath = calPath;
    if (pCal->load(qCalPath)) {
	openedCalFlag = true;
    } else {
	std::cout << "KOrgTodoPlugin: Error: Failed to load the KOrganizer" \
	    " Calendar file (" << calPath << ")." \
	    " Please edit the config file in your home directory, or" \
	    " the permisions on the calendar file to fix this problem.\n";
	return 4;
    }

    /*
    pCal = new KCal::CalendarResources();
    if (!pCal) {
	std::cout << "KOrgTodoPlugin::Initialize - ";
	std::cout << "Failed to allocate mem for CalendarResources object.\n";
	delete pKAboutData;
	return 3;
    }

    // Read the resources in from the defualt korganizer config file. This has
    // to be done before I can load in the events from the resources.
    pCal->readConfig();

    // Load all the events from the resources.
    pCal->load();
    */

    return 0;
}

/**
 * Clean up the KOrgTodoPlugin instance.
 *
 * Clean up the KOrgTodoPlugin after synchronization has been performed.
 * @return An integer representing success (zero) or failure (non-zero).
 * @retval 0 Successfully cleaned up after synchronization.
 * @retval 1 Failed to save sync ID log.
 * @retval 2 Failed to save Calendar file.
 */
int KOrgTodoPlugin::CleanUp(void) {
    int retval = 0;

    // Here, I try to save the synchronization ID log so that the next time I
    // a synchronization is performed I can load it and determine the sync IDs
    // of the items which have been deleted since the last synchronization.
    retval = SaveSyncIDLog();
    if (retval != 0) {
	std::cout << "KOrgTodoPlugin: Error: Failed to save sync ID log (";
	std::cout << retval << ")." << std::endl;
	retval = 1;
    }

    // Here I attempt to save and close the Calendar file.
    if (openedCalFlag) {
	if (!pCal->save(qCalPath)) {
	    std::cout << "KOrgTodoPlugin: Error: Failed to save calendar. ";
	    std::cout << "This means that your synchronization on the ";
	    std::cout << "Desktop side didn't happen.\n";
	    retval = 2;
	}
	pCal->close();
    }

    /*
    if (pCal) {
	pCal->save();
	pCal->close();
    }
    */

    if (pKAboutData)
	delete pKAboutData;

    return retval;
}

/**
 * Get all the Todo items.
 *
 * Get all the Todo items existing within the KOrganizer.
 * @return A list of all the Todo Items.
 */
TodoItemType::List KOrgTodoPlugin::GetAllTodoItems(void) {
    std::cout << "Entered the GetAllTodoItems() function.\n";
    TodoItemType::List todoItemList;
    KCal::Todo::List kcalTodoList;
    KCal::Todo::List::iterator kcalIt;
    TodoItemType newItem;
    std::cout << "Created all function scoped variables.\n";

    // Obtain a list of all the Todo items within the KCal object.
    //kcalTodoList = calendar.rawTodos();
    kcalTodoList = pCal->rawTodos();

    std::cout << "Obtained todo list from calendar.\n";

    kcalIt = kcalTodoList.begin();

    std::cout << "Got the beginning of the list.\n";

    // Here I handle the creation of the modified and new item lists. I do so
    // by iterating through the todo items in the calendar and comparing their
    // time of creation and time of last modification to the last time of
    // synchronization to see if they are newer, or newly modified. Then given
    // the case I add the items to the proper list so that it may be returned
    // later.
    if (!kcalTodoList.empty()) {
	std::cout << "Calendar list is not empty.\n";
	for (kcalIt = kcalTodoList.begin(); kcalIt != kcalTodoList.end();
	     ++kcalIt)
	{
	    std::cout << "Setting pKcalTodo to the item pointer.\n";
	    KCal::Todo *pKcalTodo = *kcalIt;
	    std::cout << "Set the pKcalTodo item.\n";
	    newItem = ConvKCalTodo(pKcalTodo);
	    todoItemList.push_front(newItem);
	}
    }

    std::cout << "Exiting the GetAllTodoItems() function.\n";

    return todoItemList;
}

/**
 * Get the new Todo items.
 *
 * Get the Todo items that are newer than the last time of synchronized.
 * @param lastTimeSynced last time synchronized, as seconds since Epoch.
 * @return List of the Todo items which are new (created after the last
 * synchronization).
 */
TodoItemType::List KOrgTodoPlugin::GetNewTodoItems(time_t lastTimeSynced) {
    int retval;

    if (obtainedSyncLists)
	return newTodoItemList;
    else {
	std::cout << "GetNewTodoItems: Called GetAllTodoSyncItems.\n";
	retval = GetAllTodoSyncItems(lastTimeSynced, newTodoItemList,
				     modTodoItemList, delTodoItemIdList);
	std::cout << "GetNewTodoItems: GetAllTodoSyncItems returned.\n";
    }

    return newTodoItemList;
}

/**
 * Get the modified Todo items.
 *
 * Obtain the Todo items that were modified after the last synchronization.
 * @param lastTimeSynced last time synchronized, as seconds since Epoch.
 * @return List of the Todo items that have been modified since the last
 * synchronization.
 */
TodoItemType::List KOrgTodoPlugin::GetModTodoItems(time_t lastTimeSynced) {
    int retval;

    if (obtainedSyncLists)
	return modTodoItemList;
    else {
	retval = GetAllTodoSyncItems(lastTimeSynced, newTodoItemList,
				     modTodoItemList, delTodoItemIdList);
    }

    return modTodoItemList;
}

/**
 * Get the deleted Todo Item IDs.
 * 
 * Obtain the IDs of the Todo items that were deleted some point after the
 * last synchronization.
 * @param lastTimeSynced last time synchronized, as seconds since Epoch.
 * @return List of the Todo item IDs of items that have been deleted since the
 * last synchronization.
 */
SyncIDListType KOrgTodoPlugin::GetDelTodoItemIDs(time_t lastTimeSynced) {
    int retval;

    if (obtainedSyncLists)
	return delTodoItemIdList;
    else {
	retval = GetAllTodoSyncItems(lastTimeSynced, newTodoItemList,
				     modTodoItemList, delTodoItemIdList);
    }

    return delTodoItemIdList;
}

/**
 * Add the Todo items.
 *
 * Add the possed Todo items to the KOrganizer Todo list.
 * @param todoItems List of Todo items to add.
 * @return An integer representing success (zero) or failure (non-zero).
 * @retval 0 Successfully added the items to the KOrg Todo list.
 * @retval 1 Failed to allocate memory for one of the todo items.
 * @retval 2 Failed to add on of the todo items to the KOrg Todo list.
 * @retval 3 Failed to open the calendar file, hence, no adding.
 */
int KOrgTodoPlugin::AddTodoItems(TodoItemType::List todoItems) {
    TodoItemType::List::iterator it;
    TodoItemType curItem;
    KCal::Todo *pKCalTodo;
    bool tmpBool;
    std::string funcName;

    funcName = "KOrgTodoPlugin::AddTodoItems - ";

    std::cout << funcName << "Created the function scoped variables.\n";

    // If the calendar was not opened then I want to return notifying the
    // client application of it.
    /*
    if (!openedCalFlag)
	return 3;
    */

    for (it = todoItems.begin(); it != todoItems.end(); it++) {
	std::cout << funcName << "Began an iter of one of the todo items.\n";
	curItem = *it;
	std::cout << funcName << "Set current item using iterator.\n";

	pKCalTodo = ConvTodoItemType(&curItem);
	std::cout << funcName << "Converted the TodoItem to KCal Todo Item.\n";
	if (pKCalTodo) {
	    std::cout << funcName << "Adding " << pKCalTodo->description() << \
		".\n";
	    tmpBool = pCal->addTodo(pKCalTodo);
	    if (!tmpBool) {
		std::cout << funcName << "Failed to add item to calendar.\n";
		delete pKCalTodo;
		return 2;
	    } else {
		std::cout << funcName << "Added Todo item to calendar.\n";
	    }
	} else {
	    std::cout << funcName << "Failed to alloc space for todo item.\n";
	    return 1;
	}
    }

    return 0;
}

/**
 * Modify the Todo items.
 *
 * Modify the Todo items of the KOrganizer Todo list with the values contained
 * in the list of passed Todo items.
 * @param todoItems List of Todo items to use as new data for update.
 * @return An integer representing success (zero) or failure (non-zero).
 * @retval 0 Success.
 * @retval 3 Failed to open the calendar file, hence, no modifying.
 */
int KOrgTodoPlugin::ModTodoItems(TodoItemType::List todoItems) {
    TodoItemType::List::iterator it;
    TodoItemType curTodoItem;
    KCal::Todo::List kcalTodoList;
    KCal::Todo::List::iterator kcalIt;

    /*
    // If the calendar was not opened then I want to return notifying the
    // client application of it.
    if (!openedCalFlag)
	return 3;
    */

//    kcalTodoList = calendar.rawTodos();
    kcalTodoList = pCal->rawTodos();

    for (it = todoItems.begin(); it != todoItems.end(); it++) {
	curTodoItem = (*it);

	for (kcalIt = kcalTodoList.begin(); kcalIt != kcalTodoList.end();
	     kcalIt++)
	{
	    KCal::Todo *pKcalTodo = *kcalIt;

	    // If the SyncID mathes then I want to remove this item from the
	    // KOrganizer todo calendar file.
	    if ((unsigned long int)pKcalTodo->pilotId() == 
		curTodoItem.GetSyncID()) {
		// Perform the actual modification of the item now that it
		// has been found.
		UpdateKCalTodoItem(pKcalTodo, curTodoItem);
	    }
	}
    }

    return 0;
}

/**
 * Delete the Todo items.
 *
 * Delete the Todo items that have sync IDs contained in the passed list.
 * @param todoItemIDs The Todo Item IDs of the items to remove.
 * @return An integer representing success (zero) or failure (non-zero).
 * @retval 0 Success.
 * @retval 3 Failed to open the calendar file. Hence, no deleting.
 */
int KOrgTodoPlugin::DelTodoItems(SyncIDListType todoItemIDs) {
    std::cout << "Entered the DelTodoItems function.\n";
    SyncIDListType::iterator it;
    std::cout << "Created SyncIDListType iterator.\n";
    //KCal::Todo::List kcalTodoList;
    KCal::Todo::List *pKCalTodoList;
    pKCalTodoList = NULL;
    std::cout << "Created pKCalTodoList object.\n";
    KCal::Todo::List::iterator kcalIt;
    std::cout << "Created function scoped variables.\n";

    // This line causes it to segfault, I am not sure if eliminating this is
    //ok or not. Will have to test it.
    pKCalTodoList = new KCal::Todo::List;

    std::cout << "Attempting to check for opened calendar file.\n";
    // If the calendar was not opened then I want to return notifying the
    // client application of it.
    /*
    if (!openedCalFlag)
	return 3;
    std::cout << "Checked for open calendar file.\n";
    */

    std::cout << "Obtaining KOrg Todo List.\n";
    //(*pKCalTodoList) = calendar.rawTodos();
    (*pKCalTodoList) = pCal->rawTodos();
    std::cout << "Obtained KOrg Todo List.\n";

    if ((!todoItemIDs.empty()) && (!pKCalTodoList->empty())) {
	std::cout << "Both lists are NOT empty.\n";
	for (it = todoItemIDs.begin(); it != todoItemIDs.end(); it++) {
	    for (kcalIt = pKCalTodoList->begin();
		 kcalIt != pKCalTodoList->end();
		 kcalIt++)
	    {
		KCal::Todo *pKcalTodo = *kcalIt;

		// If the SyncID mathes then I want to remove this item from
		// the KOrganizer todo calendar file.
		if ((unsigned long int)pKcalTodo->pilotId() == (*it)) {
		    //calendar.deleteTodo(pKcalTodo);
		    pCal->deleteTodo(pKcalTodo);
		}
	    }
	}
    }

    return 0;
}

/**
 * Map the item IDs.
 *
 * Map the unique identifiers between the Zaurus and KOrganizer.
 * @return An integer representing sucess (zero) or failure (non-zero).
 */
int KOrgTodoPlugin::MapItemIDs(TodoItemType::List todoItems) {
    TodoItemType::List::iterator it;
    TodoItemType curTodoItem;
    QString actAppId;
    KCal::Todo *pKcalTodo;

    // If the calendar was not opened then I want to return notifying the
    // client application of it.
    /*
    if (!openedCalFlag)
	return 3;
    */

    for (it = todoItems.begin(); it != todoItems.end(); it++) {
	curTodoItem = (*it);

	actAppId = curTodoItem.GetAppID().c_str();

	//pKcalTodo = calendar.todo(actAppId);
	pKcalTodo = pCal->todo(actAppId);

	pKcalTodo->setPilotId(curTodoItem.GetSyncID());

	std::cout << "Mapped KCal UID: " << curTodoItem.GetAppID();
	std::cout << " to Zaurus UID: " << curTodoItem.GetSyncID();
	std::cout << std::endl;
    }

    return 0;

}

/**
 * Get the plugin description.
 *
 * Obtain the description of the plugin.
 * @return The description of the plugin.
 */
std::string KOrgTodoPlugin::GetPluginDescription(void) const {
    std::string descStr;
    descStr.assign("A plugin which provides the capability of " \
		   "synchronization of the Zaurus Todo PIM application " \
		   "with the KOrganizer PIM application's Todo list.");
    return descStr;
}

/**
 * Get the plugin name.
 *
 * Obtain the name of the plugin.
 * @return The name of the plugin.
 */
std::string KOrgTodoPlugin::GetPluginName(void) const {
    std::string nameStr;
    nameStr.assign("KOrganizer Todo Sync Plugin");
    return nameStr;
}

/**
 * Get the plugin author.
 *
 * Obtain the plugin's author.
 * @return The name of the author of the plugin.
 */
std::string KOrgTodoPlugin::GetPluginAuthor(void) const {
    std::string authorStr;
    authorStr.assign("Andrew De Ponte (cyphactor@socal.rr.com)");
    return authorStr;
}

/**
 * Get the plugin version.
 *
 * Obtain the plugin's version.
 * @return The version of the plugin.
 */
std::string KOrgTodoPlugin::GetPluginVersion(void) const {
    std::string versionStr;
    versionStr.assign(TODO_PLUGIN_VERSION);
    return versionStr;
}

/**
 * Obtain all the sync data from KOrganizer for the Todo synchronization.
 *
 * Obtain all the sync data from KOrganizer for the Todo synchronization. This
 * includes all the New, Modified, and Deleted Todo item information so that
 * the synchronization can be performed.
 * @return An integer representing success (zero) or failure (non-zero).
 * @retval 0 Success.
 * @retval 1 Failed because calendar file was never opened.
 * @retval 2 Failed to allocate memory for log file content.
 * @retval 3 Failed to read expected num of sync IDs.
 */
int KOrgTodoPlugin::GetAllTodoSyncItems(time_t lastTimeSynced,
					TodoItemType::List &newItemList,
					TodoItemType::List &modItemList,
					SyncIDListType &delItemIdList)
{
    // Variables used to get the New, and Modified Todo Items.
    KCal::Todo::List kcalTodoList;
    KCal::Todo::List::iterator kcalIt;
//    QDateTime lastSynced;
    TodoItemType newItem;

    // Variables used to get the Deleted Todo Items.
    std::fstream fin, fout;
    bool foundSyncID = false;
    std::string tmpPath;
    unsigned long int numSyncIDs;
    unsigned long int numSyncIDsRead = 0;
    unsigned long int *pSyncIDs;
    unsigned long int syncCount;

    tmpPath = homeDir;
    tmpPath.append("/.KOrgTodoPlugin.log");

    // If the calendar was never opened then return with no data so nothing is
    // synchronized.
    /*
    if (!openedCalFlag)
	return 1;
    */

//    lastSynced.setTime_t(lastTimeSynced);

    // Obtain a list of all the Todo items within the KCal object.
    //kcalTodoList = calendar.rawTodos();
    kcalTodoList = pCal->rawTodos();


    std::cout << "GetAllTodoSyncItems: Got Raw Todos.\n";

    // Here I handle the creation of the modified and new item lists. I do so
    // by iterating through the todo items in the calendar and comparing their
    // time of creation and time of last modification to the last time of
    // synchronization to see if they are newer, or newly modified. Then given
    // the case I add the items to the proper list so that it may be returned
    // later.
    if (!kcalTodoList.empty()) {
	std::cout << "The kcalTodoList is not empty.\n";
	for (kcalIt = kcalTodoList.begin(); kcalIt != kcalTodoList.end();
	     ++kcalIt)
	{
	    std::cout << "Setting pKcalTodo to the item pointer.\n";
	    KCal::Todo *pKcalTodo = *kcalIt;
	    std::cout << "Set the pKcalTodo item.\n";

	    time_t tempTime;

	    std::cout << "Created Str: " << pKcalTodo->created().toString();
	    std::cout << std::endl;
	    tempTime = ConvQDateTime(pKcalTodo->created());
	    std::cout << "Created: " << ctime((const time_t *)&tempTime);
	    std::cout << " Last Synced: " << ctime((const time_t *)&lastTimeSynced) << std::endl;
	    if ((ConvQDateTime(pKcalTodo->created()) > lastTimeSynced) && (pKcalTodo->pilotId() == 0)) {
		std::cout << "Created after lastSynced.\n";
		newItem = ConvKCalTodo(pKcalTodo);
		std::cout << "Converted KCal Todo.\n";
		newItemList.push_front(newItem);
		std::cout << "Added new item to list.\n";
	    } else if ((ConvQDateTime(pKcalTodo->lastModified()) > lastTimeSynced) && (pKcalTodo->pilotId() != 0)) {
		std::cout << "Modified after lastSynced.\n";
		newItem = ConvKCalTodo(pKcalTodo);
		std::cout << "Converted KCal Todo.\n";
		modItemList.push_front(newItem);
		std::cout << "Added modified item to list.\n";
	    }
	    std::cout << "GetAllTodoSyncItems: Ended one iteration.\n";
	}
	std::cout << "Exited for loop.\n";
    }

    std::cout << "GetAllTodoSyncItems: Created New and Mod lists.\n";

    // Here I handle the creation of the deletion list. The idea behind the
    // deletion list is that a list exist containing all the SyncIDs (UIDs) of
    // the items that have been removed from the calendar since the last time
    // of synchronization.
    // 
    // I do this by storeing a list of the SyncIDs contained within the
    // current list of Todo items to a hidden file in the executing users home
    // directory. The purpose for this is so that the next time they sync the
    // file can be loaded and if I fail to find any of those SyncIDs in the
    // current calendar Todo list then I know that, that I item has since been
    // removed.

    std::cout << "Attempting to open log.\n";
    std::cout << "tmpPath = " << tmpPath << std::endl;
    fin.open(tmpPath.c_str(), std::fstream::in);
    std::cout << "Finished open log call.\n";
    if (fin.is_open()) {

	std::cout << "Opened the log file.\n";

	// Obtain the number of sync ids contained within the log file so I
	// know how much memory to allocate and I know how many bytes to read
	// in.
	fin.read((char *)&numSyncIDs, 4);

	// Here, I check to see if the .KOrgTodoPlugin.log has hit the end of
	// file at this point. If it has hit the end of file then we know that
	// the log file does not contain valid data and possibly no data. In
	// either case I want the numSyncIDs to represent 0 because I want it
	// to act as if there were no items in the file.
	if (!fin.good()) {
	    numSyncIDs = 0;
	}

	std::cout << "Read the num of sync ids in log (" << numSyncIDs;
	std::cout << ").\n";

	// Dynamically allocate an array large enough to hold all the sync ids
	// stored in the log file.
	if (numSyncIDs) {
	    pSyncIDs = new unsigned long int[numSyncIDs];
	    if (pSyncIDs == NULL)
		return 2;
	}

	// Load all the sync ids into the dynamically allocated array.
	for (syncCount = 0; (syncCount < numSyncIDs); syncCount++) {
	    fin.read((char *)&pSyncIDs[syncCount], 4);
	    // Now, while reading in the content of the file, if for some
	    // reason I hit eof before I should I want to set the numSyncIDs
	    // to the number of sync IDs successufully read in.
	    /*
	    if (fin.good()) {
		numSyncIDsRead = syncCount;
	    }
	    */
	}

	numSyncIDsRead = syncCount;

	std::cout << "Read in " << numSyncIDsRead << " sync ids.\n";

	// At this point the array should be filled with all of the sync
	// ids. Hence, I should be able to continue with the process.
	for (syncCount = 0; syncCount < numSyncIDsRead; syncCount++) {
	    foundSyncID = false;

	    if (!kcalTodoList.empty()) {
		std::cout << "KCal Todo list is not empty.\n";
		for (kcalIt = kcalTodoList.begin();
		     kcalIt != kcalTodoList.end();
		     kcalIt++)
		{
		    KCal::Todo *pKcalTodo = *kcalIt;
		    
		    if ((unsigned long int)pKcalTodo->pilotId() ==
			pSyncIDs[syncCount]) {
			foundSyncID = true;
		    }
		}
	    }

	    if (!foundSyncID) {
		std::cout << "sync id not found.\n";
		delItemIdList.push_front(pSyncIDs[syncCount]);
		std::cout << "Added to del item list.\n";
	    }
	    std::cout << "Finished an iteration (" << syncCount << ").\n";
	}

	std::cout << "Finished sync id loop.\n";

	if (numSyncIDs) {
	    delete [] pSyncIDs;
	    std::cout << "Freed sync id list memory.\n";
	}
    }

    std::cout << "Attempting to close log file.\n";

    fin.close();

    std::cout << "Closed log file.\n";

    obtainedSyncLists = true;

    if (numSyncIDsRead != numSyncIDs) {
	return 3;
    }

    std::cout << "GetAllTodoSyncITems: Exiting function.\n";

    // Return in succes.
    return 0;
}

/**
 * Save the SyncID Log.
 *
 * Save the SyncID log (a log file containing a record of all the items
 * SyncIDs). This is used to save the current state of the SyncIDs of
 * KOrganizer's Todo list for access at a later point. Specifically so that it
 * can be used to check for removal of items for the purpose of generating the
 * deltoodItemIdList.
 * @return An integer representing success (zero) or failure (non-zero).
 * @retval 0 Success.
 * @retval 1 Failed to open the file for output.
 */
int KOrgTodoPlugin::SaveSyncIDLog(void) {
    std::string tmpPath = homeDir;
    std::fstream fout;
    unsigned long int numSyncIDs;
    unsigned long int tmpSyncID;
    KCal::Todo::List kcalTodoList;
    KCal::Todo::List::iterator kcalIt;

    tmpPath.append("/.KOrgTodoPlugin.log");

    // Obtain a list of all the Todo items within the KCal object.
    //kcalTodoList = calendar.rawTodos();
    kcalTodoList = pCal->rawTodos();


    fout.open(tmpPath.c_str(), std::fstream::out);
    if (!fout.is_open())
	return 1;

    // Count the number of items that have a pilotId() (rather SyncID) greater
    // than zero. Write this value of the number SyncIDs.
    numSyncIDs = 0;
    for (kcalIt = kcalTodoList.begin(); kcalIt != kcalTodoList.end(); kcalIt++)
    {
	KCal::Todo *pKcalTodo = *kcalIt;
	if (pKcalTodo->pilotId() != 0)
	    numSyncIDs++;
    }
    fout.write((const char *)&numSyncIDs, 4);

    // Loop through the list of todo Items and write their values in series if
    // the value is larger than zero.
    for (kcalIt = kcalTodoList.begin(); kcalIt != kcalTodoList.end(); kcalIt++)
    {
	KCal::Todo *pKcalTodo = *kcalIt;
	tmpSyncID = pKcalTodo->pilotId();
	if (tmpSyncID != 0)
	    fout.write((const char *)&tmpSyncID, 4);
    }

    fout.close();

    return 0;
}

/**
 * Convert a KCal::Todo object into a common TodoItemType object.
 *
 * Convert a KCal::Todo object into a common TodoItemType object so that the
 * plugin interface can use the common format to synchronize the data.
 * @param pKcalTodo Pointer to the KCal::Todo object to convert.
 * @return A TodoItemType object containing the converted data.
 */
TodoItemType KOrgTodoPlugin::ConvKCalTodo(KCal::Todo *pKcalTodo) {
    TodoItemType todoItem;
    QCString appId;

    // Here I convert all the common data.

    // Set the attribute.
    todoItem.SetAttribute((unsigned char)0);

    // Set the created time to zero signifying that it has not been set.
    todoItem.SetCreatedTime(ConvQDateTime(pKcalTodo->created()));

    // Set the modified time to zero signifying that it has not been set.
    todoItem.SetModifiedTime(ConvQDateTime(pKcalTodo->lastModified()));

    // Set the sync id.
    todoItem.SetSyncID((unsigned long int)pKcalTodo->pilotId());

    // Set the application id.
    appId = pKcalTodo->uid().utf8();
    todoItem.SetAppID((std::string)appId);

    // Below I convert all the Todo specific data.

    // Set the item category. Now in this case the Zaurus Todo item only has
    // one category and in the KOrganizer Todo items they can have multiple
    // categories. In this case I have decided to only use the first category
    // of the KOrganizers categories list.
    QStringList kOrgCatList;
    QStringList::Iterator catIt;
    QCString category;

    kOrgCatList = pKcalTodo->categories();
    catIt = kOrgCatList.begin();
    category = (*catIt).utf8();
    todoItem.SetCategory((std::string)category);

    // Set Start Date
    if (pKcalTodo->hasStartDate()) {
	todoItem.SetStartDate(ConvQDateTime(pKcalTodo->dtStart()));
    } else {
	todoItem.SetStartDate(0);
    }

    // Set Due Date
    if (pKcalTodo->hasDueDate()) {
	todoItem.SetDueDate(ConvQDateTime(pKcalTodo->dtDue()));
    } else {
	todoItem.SetDueDate(0);
    }

    // Set Completed Date
    if (pKcalTodo->hasCompletedDate()) {
	todoItem.SetCompletedDate(ConvQDateTime(pKcalTodo->completed()));
    } else {
	todoItem.SetCompletedDate(0);
    }

    // Set the item progress status.
    if (pKcalTodo->isCompleted())
	todoItem.SetProgressStatus(0);
    else
	todoItem.SetProgressStatus(1);

    // Set the item priority. I got lucky in this case the KOrganizer todo
    // list uses the same range for priority as the Zaurus. All I had to do
    // was type cast the int into a unsigned char because the methods of
    // storage are different.
    todoItem.SetPriority((unsigned char)pKcalTodo->priority());

    // Set Description.
    QCString descStr;

    descStr = (pKcalTodo->summary()).utf8();
    todoItem.SetDescription((std::string)descStr);

    // Set Notes.
    QCString notesStr;

    notesStr = (pKcalTodo->description()).utf8();
    todoItem.SetNotes((std::string)notesStr);

    return todoItem;
}

/**
 * Convert a TodoItemType object into a KOrganizers KCal::Todo object.
 *
 * Convert a TodoItemType object into a KCal::Todo object so that the
 * the an item that was of type TodoItemType can now be added to the
 * KOrganizer Todo list. Note: This dynamically allocates the memory for the
 * new KCal::Todo item. Hence, if it is not added to the calender it needs to
 * be deallocated at some point or there will be a memory leak.
 * @param pTodoItem Pointer to the TodoItemType object to convert.
 * @return A pointer to the new KCal::Todo object.
 * @retval NULL Failed to allocate memory for the new object.
 */
KCal::Todo *KOrgTodoPlugin::ConvTodoItemType(TodoItemType *pTodoItem) {
    KCal::Todo *pKCalTodo;
    QDateTime tmpTime;
    QString tmpStr;
    QString uId;

    // Create the instance of the object for the list of todo items stored in
    // the calendar. If failed to allocate the memory for it then return NULL.
    pKCalTodo = new KCal::Todo;
    if (!pKCalTodo)
	return pKCalTodo;

    UpdateKCalTodoItem(pKCalTodo, *(pTodoItem));

    /*
    std::cout << "ConvTodoItemType\n";

    // Go through all the todo data items and convert them into a KCalTodo
    // data items. 

    // Set the created time.
    tmpTime.setTime_t(pTodoItem->GetCreatedTime());
    pKCalTodo->setCreated(tmpTime);

    // Set the modified time.
    tmpTime.setTime_t(pTodoItem->GetModifiedTime());
    pKCalTodo->setLastModified(tmpTime);

    // Set the sync ID (pilot id).
    pKCalTodo->setPilotId((int)pTodoItem->GetSyncID());

    // Now I convert the Todo specific data items.
    
    // Set the category.
    tmpStr = pTodoItem->GetCategory();
    pKCalTodo->setCategories(tmpStr);
    std::cout << "Category: " << tmpStr << std::endl;

    // Set the start date.
    if (pTodoItem->GetStartDate() != 0) {
	tmpTime.setTime_t(pTodoItem->GetStartDate());
	pKCalTodo->setDtStart(tmpTime);
	pKCalTodo->setHasStartDate(true);
    } else {
	pKCalTodo->setHasStartDate(false);
    }

    // Set the due date.
    if (pTodoItem->GetDueDate() != 0) {
	tmpTime.setTime_t(pTodoItem->GetDueDate());
	pKCalTodo->setDtDue(tmpTime);
	pKCalTodo->setHasDueDate(true);
    } else {
	pKCalTodo->setHasDueDate(false);
    }

    // Set the completed date.
    if (pTodoItem->GetCompletedDate() != 0) {
	tmpTime.setTime_t(pTodoItem->GetCompletedDate());
	pKCalTodo->setCompleted(tmpTime);
    }

    // Set the progress status.
    if (pTodoItem->GetProgressStatus() == 0) {
	pKCalTodo->setCompleted(true);
    } else {
	pKCalTodo->setCompleted(false);
    }

    // Set the priority.
    pKCalTodo->setPriority((int)pTodoItem->GetPriority());
    std::cout << "Priority: " << pTodoItem->GetPriority() << std::endl;

    // Set the description (KOrg Summary).
    tmpStr = pTodoItem->GetDescription();
    pKCalTodo->setSummary(tmpStr);

    // Set the notes (KOrg Description).
    tmpStr = pTodoItem->GetNotes();
    pKCalTodo->setDescription(tmpStr);
    */

    return pKCalTodo;
}

/**
 * Update a KCal TodoItem with values in TodoItemType object.
 *
 * This basically sets the values of the KCal Todo item to the proper values
 * based on the values of the TodoItemType object.
 * @param pKCalTodo Pointer to the KCal::Todo item to update.
 * @param todoItem The TodoItemType object to get data from for the update.
 */
void KOrgTodoPlugin::UpdateKCalTodoItem(KCal::Todo *pKCalTodo,
					TodoItemType todoItem) {
    QDateTime tmpTime;
    QString tmpStr;
    QString uId;
    TodoItemType *pTodoItem;

    struct tm brTime;
    char buff[256];
    time_t zTime;

    std::string funcName;

    funcName.assign("KOrgTodoPlugin::UpdateKCalTodoItem - ");

    pTodoItem = &todoItem;

    // Go through all the todo data items and convert them into a KCalTodo
    // data items. 

    // Set the created time.
    tmpTime.setTime_t(pTodoItem->GetCreatedTime());
    pKCalTodo->setCreated(tmpTime);

    std::cout << funcName << "-=Created Time=-\n";
    zTime = pTodoItem->GetCreatedTime();
    localtime_r(&zTime, &brTime);
    asctime_r(&brTime, buff);
    std::cout << funcName << "pTodoItem: " << buff << std::endl;

    tmpStr = tmpTime.toString("dd-MM-yyyy hh:mm:ss");
    std::cout << funcName << "pKCalTodo: " << tmpStr << std::endl;

    // Set the modified time.
    tmpTime.setTime_t(pTodoItem->GetModifiedTime());
    pKCalTodo->setLastModified(tmpTime);

    // Set the sync ID (pilot id).
    pKCalTodo->setPilotId((int)pTodoItem->GetSyncID());

    // Now I convert the Todo specific data items.
    
    // Set the category.
    tmpStr = pTodoItem->GetCategory();
    pKCalTodo->setCategories(tmpStr);

    // Set the start date.
    if (pTodoItem->GetStartDate() != 0) {
	tmpTime.setTime_t(pTodoItem->GetStartDate());
	pKCalTodo->setDtStart(tmpTime);
	pKCalTodo->setHasStartDate(true);
    } else {
	pKCalTodo->setHasStartDate(false);
    }

    // Set the due date.
    if (pTodoItem->GetDueDate() != 0) {
	tmpTime.setTime_t(pTodoItem->GetDueDate());
	pKCalTodo->setDtDue(tmpTime);
	pKCalTodo->setHasDueDate(true);
    } else {
	pKCalTodo->setHasDueDate(false);
    }

    // Set the completed date.
    if (pTodoItem->GetCompletedDate() != 0) {
	tmpTime.setTime_t(pTodoItem->GetCompletedDate());
	pKCalTodo->setCompleted(tmpTime);
    }

    // Set the progress status.
    if (pTodoItem->GetProgressStatus() == 0) {
	pKCalTodo->setCompleted(true);
    } else {
	pKCalTodo->setCompleted(false);
    }

    // Set the priority.
    pKCalTodo->setPriority((int)pTodoItem->GetPriority());

    // Set the description (KOrg Summary).
    tmpStr = pTodoItem->GetDescription();
    std::cout << funcName << "tmpStr (desc) = " << tmpStr << ".\n";
    pKCalTodo->setSummary(tmpStr);

    // Set the notes (KOrg Description).
    tmpStr = pTodoItem->GetNotes();
    std::cout << funcName << "tmpStr (notes) = " << tmpStr << ".\n";
    pKCalTodo->setDescription(tmpStr);
}

/**
 * Convert a QDateTime object to secs since Epoch.
 *
 * Convert a QDateTime object into the number of seconds since Epoch (00:00:00
 * UTC, January 1, 1970), measured in seconds.
 * @param dateTime The QDateTime object to be converted to secs since Epoch.
 * @return The converted QDateTime in seconds since Epoch.
 */
time_t KOrgTodoPlugin::ConvQDateTime(QDateTime dateTime) {
    /*
    struct tm tmpTime;
    QDate date;
    QTime time;

    date = dateTime.date();
    time = dateTime.time();

    tmpTime.tm_sec = time.second();
    tmpTime.tm_min = time.minute();
    tmpTime.tm_hour = time.hour();
    tmpTime.tm_mday = date.day();
    tmpTime.tm_mon = (date.month() - 1);
    tmpTime.tm_year = (date.year() - 1900);
    tmpTime.tm_wday = (date.dayOfWeek() - 1);
    tmpTime.tm_yday = date.dayOfYear();
    tmpTime.tm_isdst = -1;

    return mktime(&tmpTime);
    */

    return dateTime.toTime_t();
}
