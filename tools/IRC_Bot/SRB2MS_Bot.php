<?php
/* Based on parts of Example.php from SmartIRC
 * Example.php Copyright (c) 2002-2003 Mirco "MEEBEY" Bauer <mail@meebey.net> <http://www.meebey.net>
 * SRB2MS.php Copyright (c) 2004 by Logan Arias of Sonic Team JR.
 *
 * the Rest is Copyright (c) 2004 Logan Arias <Logan.GBA@gmail.net>
 * This is LGPL! that means you can change and use as you like
 * 
 * Full LGPL License: <http://www.meebey.net/lgpl.txt>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
include_once('./SmartIRC.php');

class SRB2MS_BOT
{

	function ListGamesPM(&$irc, &$data)
	{
		$fd = fsockopen("srb2.servegame.org", 28900, $errno, $errstr, 5);
		if ($fd)
		{
			$buff = "000012360000";
			fwrite($fd, $buff);
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, 'Running SRB2 NetGames ');
			while (1)
			{
				$content=fgets($fd, 13); // skip 13 first bytes
				$content=fgets($fd, 1024);
				$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "$content");
				//echo "$content";
				if (feof($fd)) break;
			}
		}
		else
		{
			$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, 'The SRB2 MasterServer is not up');
		}
	}

	function ListGamesChan(&$irc, &$data)
	{
		$fd = fsockopen("192.168.0.2", 28900, $errno, $errstr, 5);
		if ($fd)
		{
			$buff = "000012360000";
			fwrite($fd, $buff);
			$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, 'Running SRB2 NetGames ');
			while (1)
			{
				$content=fgets($fd, 13); // skip 13 first bytes
				$content=fgets($fd, 1024);
				$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$content");
				//echo "$content";
				if (feof($fd)) break;
			}
		}
		else
		{
			$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, 'The SRB2 MasterServer is not up');
		}
	}

	function BeenInvited(&$irc, &$data)
	{
		if ($data->message == '#srb2fun')
			$irc->join($data->message);
		
		foreach ($irc->channel['#srb2fun']->ops as $invitenick => $key )
		{
			if ($data->nick == $invitenick)
			{
				$irc->join($data->message);
				return;
			}
			if ($data->nick == 'Logan_GBA')
			{
				$irc->join($data->message);
				return;
			}
		}
		foreach ($irc->channel['#srb2fun']->users as $nick => $key ) 
		{
			if ($data->nick == $nick)
			{
				$irc->kick('#srb2fun', $data->nick, "Sorry, but I don't like you, or your channel, $data->message");
				return;
			}
		}
		//$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "Sorry, but I don't like you, or your channel, $data->message");
	}
	
	function Talk(&$irc, &$data)
	{
		foreach ($irc->channel['#srb2fun']->ops as $nick => $key) 
		{
			if ($data->nick == $nick)
			{
				$msg = str_replace("!talk ", '', "$data->message");
				$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun', $msg);
				return;
			}
		}
		foreach ($irc->channel['#srb2fun']->users as $nick => $key) 
		{
			if ($data->nick == $nick)
			{
				$irc->kick('#srb2fun', $data->nick, 'Stop talking to me!');
				return;
			}
		}		
	}
	
	function Action(&$irc, &$data)
	{
		foreach ($irc->channel['#srb2fun']->ops as $nick => $key) 
		{
			if ($data->nick == $nick)
			{
				$msg = str_replace("!action ", '', "$data->message");
				$irc->message(SMARTIRC_TYPE_ACTION, '#srb2fun', $msg);
				return;
			}
		}
		foreach ($irc->channel['#srb2fun']->users as $nick => $key) 
		{
			if ($data->nick == $nick)
			{
				$irc->kick('#srb2fun', $data->nick, 'Stop talking to me!');
				return;
			}
		}	
	}
	
	function Kick(&$irc, &$data)
	{
		foreach ($irc->channel['#srb2fun']->ops as $nick => $key) 
		{
			if ($data->nick == $nick)
			{
				$msg = str_replace("!kick ", '', "$data->message");
				$irc->kick('#srb2fun', $msg);
				return;
			}
		}
		foreach ($irc->channel['#srb2fun']->users as $nick => $key) 
		{
			if ($data->nick == $nick)
			{
				$irc->kick('#srb2fun', $data->nick, 'Stop talking to me!');
				return;
			}
		}		
	}

	function DebugPM(&$irc, &$data)
	{
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "from:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->from");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "nick:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->nick");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "ident:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->ident");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "host:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->host");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "channel:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->channel");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "message:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->message");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "type:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->type");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "rawmessage:");
		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->rawmessage");
	}

}

// the Bot is the Class SRB2MS_BOT
$bot = &new SRB2MS_BOT();
// Create SmartIRC, and name it as <network>irc for use to, well, do irc stuff ^_^
$esperirc = &new Net_SmartIRC();
//no Excess Flood!
//$esperirc->setSendDelay(300);
// To Debug or Not
//$esperirc->setDebug(SMARTIRC_DEBUG_IRCMESSAGES);
// Do connect to the Internet
$esperirc->setUseSockets(TRUE);
// Register the trigger !listgames as to call ListGames with the class of SRB2MS_BOT
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!gamelist$', $bot, 'ListGamesPM');
$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!gamelist$', $bot, 'ListGamesChan');
$esperirc->registerActionhandler(SMARTIRC_TYPE_INVITE, '', $bot, 'BeenInvited');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_ALL, '', $bot, 'DebugPM');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!talk', $bot, 'Talk');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!action', $bot, 'Action');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!kick', $bot, 'Kick');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!ban', $bot, 'Ban');
// If you fail once, try agian
$esperirc->setAutoRetry(TRUE);
// Connect to EsperNet
$esperirc->connect('cosmos.esper.net', 6667);
// kep a list of all the users in the channel
$esperirc->setChannelSyncing(TRUE);
// Reconnect as needed
$esperirc->setAutoReconnect(TRUE);
// Use the name SRB2MS, and set other stuff
$esperirc->login('SRB2MS2', 'Net_SmartIRC Client '.SMARTIRC_VERSION.' (SRB2MS_Bot.php)', 0, 'Net_SmartIRC');
// Join #srb2fun and #srb2general
$esperirc->join((array('#srb2general,#srb2fun')));
// SRB2MS_Pass.php have "$esperirc->message(SMARTIRC_TYPE_QUERY, 'nickserv', 'identify <password>');"
include('./SRB2MS_Pass.php');
// Idle
$esperirc->listen();
// Rejoin?
$esperirc->reconnect();
?>
