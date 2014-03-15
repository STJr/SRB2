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
define('MAGPIE_DIR', './magpierss/');
// define('MAGPIE_CACHE_AGE', 30); // half-min
require_once(MAGPIE_DIR.'rss_fetch.inc');
require_once(MAGPIE_DIR.'rss_utils.inc');
$today = getdate();
$lastupdatetime = mktime($today['hours'],$today['minutes']-6,0,$today['mon'], $today['mday'], $today['year']);
$lastlisttime = mktime($today['hours'],$today['minutes']-1,0,$today['mon'], $today['mday'], $today['year']);

class SRB2MS_BOT
{

	function ListUsersPM(&$irc, &$data)
	{
		global $rssdatabase;
		if ($rssdatabase->users[0])
		{
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, 'List of RSS users');
			foreach ($rssdatabase->users as $users) 
			{
				$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "$users");
			}
		}
		else
		{
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, 'there are no RSS users');
		}
	}

	function RSSupdateuser(&$irc, &$data)
	{
		global $rssdatabase;
		if ($rssdatabase->OnList("$data->nick"))
		{
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->message, "I am trying to update $data->nick to $data->message from the list");
			if ($rssdatabase->ReplaceUser("$data->nick","$data->message"))
			{
				$irc->message(SMARTIRC_TYPE_NOTICE, $data->message, "I have changed $data->nick to $data->message on the list");
			}
			else
			{
				$irc->message(SMARTIRC_TYPE_NOTICE, $data->message, "$data->nick was not on the list");
			}
		}
	}
	
	function RSScheckuser(&$irc, &$data)
	{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "I going to check if you are on the list");
		global $rssdatabase;
		if ($rssdatabase->OnList("$data->nick"))
		{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "you are on the list");
		}
		else
		{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "you are not on the list");
		}
	}

	function RSSremoveuser(&$irc, &$data)
	{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "I am trying to remove $data->nick from the list");
		global $rssdatabase;
		if ($rssdatabase->DeleteUser("$data->nick"))
		{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "I have removed $data->nick from the list");
		}
		else
		{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "$data->nick is not on the list");
		}
	}

	function RSSadduser(&$irc, &$data)
	{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "I am trying to add you $data->nick to the list");
		global $rssdatabase;
		if ($rssdatabase->AddUser("$data->nick"))
		{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "I have added $data->nick to the list");
		}
		else
		{
		$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "$data->nick is already on the list");
		}
	}
/*
	function RSSListGamesPM(&$irc, &$data)
	{
		$content = $this->GenFeed();
		$SRB2MSRSS10 = new MagpieRSS( $content );
		if ($SRB2MSRSS10)
		{
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, 'Running SRB2 netgames ');
			foreach ($SRB2MSRSS10->items as $item) 
			{
				$description = $item['dc']['description'];
				$title = $item['title'];
				$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "Name: $title $description");
			}
		}
		else
		{
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, '.magpie_error()');
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, 'The SRB2 MasterServer RSS feed is not up');
		}
	}
*/
	function RSSListUpdate(&$irc)
	{
		global $lastupdatetime;
		global $SRB2MS_mute;
		if ($SRB2MS_mute)
		{
			return;
		}
		$RSS10content = $this->GenFeed();
		$SRB2MSRSS10 = new MagpieRSS( $RSS10content );
		if ($SRB2MSRSS10)
		{
			//$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun', 'Running SRB2 netgames ');
			foreach ($SRB2MSRSS10->items as $item) 
			{
				if ( $item['title'] == "No servers" ) 
				{
					// NOP
				}
				elseif ( $item['title'] == "No master server" ) 
				{
					// NOP
				}
				else
				{
					$published = parse_w3cdtf($item['dc']['date']);
					if ( $published >= $lastupdatetime ) 
					{
						$title = $item['title'];
						$serveraddress = $item['srb2ms']['address'];
						$serverport = $item['srb2ms']['port'];
						$serverversion = $item['srb2ms']['version'];
						//if ( $serverversion == "1.09.3" )
						//{
						//	$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun'    , "New Server, $serveraddress:$serverport Name: $title");
						//}
						//else
						{
							$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun'    , "New Server, $serveraddress:$serverport Name: $title Version: $serverversion");
						}
						//$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2general', "New Server, Name: $title $description");
					}
					else
					{
						// NOP
					}
				}
				
			}
			$today = getdate();
			// update time
			$lastupdatetime = mktime($today['hours'],$today['minutes'],0,$today['mon'], $today['mday'], $today['year']);
		}
		else
		{
			$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun', '.magpie_error()');
			$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun', 'The SRB2 MasterServer RSS feed is not up');
		}
	}
	
	function Mute(&$irc, &$data)
	{		
		foreach ($irc->channel['#srb2fun']->ops as $nick => $key )
		{
			if ($data->nick == $nick || $data->nick == 'Logan_GBA')
			{
				global $lastupdatetime;
				$today = getdate();
				// update time
				$lastupdatetime = mktime($today['hours'],$today['minutes'],0,$today['mon'], $today['mday'], $today['year']);
				$GLOBALS['SRB2MS_mute'] = TRUE;
				return;
			}
		}
	}
	
	function Unmute(&$irc, &$data)
	{		
		foreach ($irc->channel['#srb2fun']->ops as $nick => $key )
		{
			if ($data->nick == $nick || $data->nick == 'Logan_GBA')
			{
				$GLOBALS['SRB2MS_mute'] = FALSE;
				return;
			}
		}
	}


	function Mute2(&$irc, &$data)
	{		
		if ($data->nick == 'Logan_GBA')
		{
			global $lastupdatetime;
			$today = getdate();
			// update time
			$lastupdatetime = mktime($today['hours'],$today['minutes'],0,$today['mon'], $today['mday'], $today['year']);
			$GLOBALS['SRB2MS_mute'] = TRUE;
		}
	}
	
	function Unmute2(&$irc, &$data)
	{		
		if ($data->nick == 'Logan_GBA')
		{
			$GLOBALS['SRB2MS_mute'] = FALSE;
		}
	}

	function RSSListGames(&$irc, &$data)
	{
		global $lastlisttime;
		$content = $this->GenFeed();
		$today = getdate();
		//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, 'testing feed');
		//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$content");
		//$content1 = $this->Gettextfromserver();
		//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$content1");
		$SRB2MSRSS10 = new MagpieRSS( $content );
		if ($SRB2MSRSS10)
		{
			if ((mktime($today['hours'],$today['minutes']-2,0,$today['mon'], $today['mday'], $today['year'])) >= $lastlisttime)
			{
					//$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun', 'Running SRB2 netgames ');
					foreach ($SRB2MSRSS10->items as $item) 
					{
						if ( $item['title'] == "No servers" ) 
						{
							//0$description = $item['dc']['description'];
							$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "No SRB2 Games are running, so why don't you start one?");
						}
						elseif ( $item['title'] == "No master server" )
						{
							$description = $item['dc']['description'];
							$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$description");
						}
						else
						{
							$title = $item['title'];
							$serveraddress = $item['srb2ms']['address'];
							$serverport = $item['srb2ms']['port'];
							$serverversion = $item['srb2ms']['version'];
							//if ( $serverport == " 5029" )
							//{
							//	$serverport = "5029";
							//}
							//if ( $serverversion == "1.09.3" )
							//{
							//	$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun'    , "$serveraddress:$serverport Name: $title");
							//}
							//else
							{
								$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun'    , "$serveraddress:$serverport Name: $title  Version: $serverversion");
							}
							
						}
					
				}
				$today = getdate();
				$lastlisttime = mktime($today['hours'],$today['minutes'],0,$today['mon'], $today['mday'], $today['year']);
			}

		}
		else
		{
			$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, ".magpie_error()");
			$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, 'The SRB2 MasterServer RSS feed is not up');
		}
	}
	
	function RSSListGamesPM(&$irc, &$data)
	{
		global $lastlisttime;
		$content = $this->GenFeed();
		$today = getdate();
		//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, 'testing feed');
		//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$content");
		//$content1 = $this->Gettextfromserver();
		//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$content1");
		$SRB2MSRSS10 = new MagpieRSS( $content );
		if ($SRB2MSRSS10)
		{
			//if ((mktime($today['hours'],$today['minutes']-2,0,$today['mon'], $today['mday'], $today['year'])) >= $lastlisttime)
			{
					//$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun', 'Running SRB2 netgames ');
					$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "Running SRB2 netgames from http://srb2.servegame.org/");
					foreach ($SRB2MSRSS10->items as $item) 
					{
						if ( $item['title'] == "No servers" ) 
						{
							//0$description = $item['dc']['description'];
							//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "No SRB2 Games are running, so why don't you start one?");
						}
						elseif ( $item['title'] == "No master server" )
						{
							$description = $item['dc']['description'];
							//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$description");
						}
						else
						{
							$title = $item['title'];
							$serveraddress = $item['srb2ms']['address'];
							$serverport = $item['srb2ms']['port'];
							$serverversion = $item['srb2ms']['version'];
							//if ( $serverport == " 5029" )
							//{
							//	$serverport = "5029";
							//}
							//if ( $serverversion == "1.09.3" )
							//{
							//	$irc->message(SMARTIRC_TYPE_CHANNEL, '#srb2fun'    , "$serveraddress:$serverport Name: $title");
							//}
							//else
							{
								$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, "$serveraddress:$serverport Name: $title  Version: $serverversion");
							}
							
						}
					
				}
				$today = getdate();
				$lastlisttime = mktime($today['hours'],$today['minutes'],0,$today['mon'], $today['mday'], $today['year']);
			}

		}
		else
		{
			//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, ".magpie_error()");
			//$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, 'The SRB2 MasterServer RSS feed is not up');
		}
	}

	function ListGamesPM(&$irc, &$data)
	{
		$fd = fsockopen("srb2.servegame.org", 28900, $errno, $errstr, 5);
		if ($fd)
		{
			$buff = "000012360000";
			fwrite($fd, $buff);
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, 'Running SRB2 netgames ');
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
			$irc->message(SMARTIRC_TYPE_NOTICE, $data->nick, 'The SRB2 MasterServer is not up');
		}
	}

	function ListGamesChan(&$irc, &$data)
	{
		global $lastlisttime;
		$today = getdate();
		$fd = fsockopen("srb2.servegame.org", 28900, $errno, $errstr, 5);
		if ($fd)
		{
			stream_set_timeout ($fd, 5);
			$buff = "000012360000";
			fwrite($fd, $buff);
			if ( (mktime($today['hours'],$today['minutes']-5,0,$today['mon'], $today['mday'], $today['year'])) >= $lastlisttime )
			{
				$content = "";
				while (1)
				{	
					fgets($fd, 13); // skip 13 first bytes
					$content = fgets($fd, 1024);
					$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, "$content");
					$today = getdate();
					$lastlisttime = mktime($today['hours'],$today['minutes'],0,$today['mon'], $today['mday'], $today['year']);
					if (feof($fd)) break;
				}
			}
		}
		else
		{
			$irc->message(SMARTIRC_TYPE_CHANNEL, $data->channel, 'The SRB2 MasterServer is not up');
		}
	}
	
	function byebye(&$irc, &$data)
	{
		foreach ($irc->channel['#srb2fun']->ops as $quitnick => $key )
		{
			if ($data->nick == $quitnick)
			{
				$irc->setAutoReconnect(FALSE);
				$irc->setAutoRetry(FALSE);
				$irc->quit('This quit was done by' + "$data->nick");
				return;
			}
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
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "from:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->from");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "nick:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->nick");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "ident:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->ident");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "host:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->host");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "channel:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->channel");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "message:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->message");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "type:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->type");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "rawmessage:");
//		$irc->message(SMARTIRC_TYPE_QUERY, $data->nick, "$data->rawmessage");
		
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "from: $data->from");
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "nick: $data->nick");
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "ident: $data->ident");
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "host: $data->host");
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "channel: $data->channel");
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "message: $data->message");
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "type: $data->type");
		$irc->message(SMARTIRC_TYPE_QUERY, "Alam_GBC", "rawmessage: $data->rawmessage");
	}

	function GetRSS10datafromserver()
	{
		$content = "";
		$fd = fsockopen("srb2.servegame.org", 28900, $errno, $errstr, 5);
		if ($fd)
		{
			$buff = "000012400000";
			fwrite($fd, $buff);
			while (1)
			{
				fgets($fd, 13); // skip 13 first bytes
				$content .= fgets($fd, 1024);
				if (feof($fd)) break;
			}
			fclose($fd);
		}
		else
		{
			$content = '<items>';
			$content .= '<rdf:Seq>';
			$content .= '<rdf:li rdf:resource="http://srb2.servegame.org/" />';
			$content .= '</rdf:Seq>';
			$content .= '</items>';
			$content .= '</channel>';
			$content .= "\n";
			$content .= '<item rdf:about="http://srb2.servegame.org/"> ';
			$content .= '<title>No master server</title><dc:description>The master server is not running</dc:description></item>';
		}
		return "$content";
	}

	function GenFeed()
	{
		$content = '<?xml version="1.0" encoding="ISO-8859-1"?>';
		$content .= "\n";
		$content .= '<rdf:RDF';
		$content .= '  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"';
		$content .= "\n";
		$content .= '  xmlns="http://purl.org/rss/1.0/"';
		$content .= "\n";
		$content .= '  xmlns:dc="http://purl.org/dc/elements/1.1/"';
		$content .= "\n";
		$content .= '  xmlns:srb2ms="http://srb2.servegame.org/SRB2MS/elements/"';
		$content .= "\n";
		$content .= '>';
		$content .= "\n";
		$content .= "\n";
		$content .= '  <channel rdf:about="http://srb2.servegame.org/srb2ms_status.php">';
		$content .= "\n";
		$content .= '    <title>SRB2 Master Server RSS Feed</title>';
		$content .= "\n";
		$content .= '    <link>http://srb2.servegame.org/srb2ms_status.php</link>';
		$content .= "\n";
		$content .= '    <description>Playing around with RSS</description>';
		$content .= "\n";
		$content .= '    <language>en-us</language>';
		$content .= "\n";
		$feed = $this->GetRSS10datafromserver();
		$content .= "$feed";
		$content .= '</rdf:RDF>';
		return $content;
	}
}

class RSSDataBase_Class
{
	var $users = array();
	
	function OnList($User)
	{
		$userbeenfound = FALSE;
		foreach ($this->users as $Userlist) 
		{
			if ($Userlist == $User) {
			$userbeenfound = TRUE; }
		}
		return $userbeenfound;
	}

	function AddUser($User)
	{
		$userbeenfound = FALSE;
		foreach ($this->users as $Userlist) 
		{
			if ($Userlist == $User) {
			$userbeenfound = TRUE; }
		}
		if (!$userbeenfound) {
		$this->users[] = $User; 
		}
		return !$userbeenfound;
	}
	
	function DeleteUser($User)
	{
		$userbeenfound = FALSE;
		foreach ($this->users as $index => $Userlist) 
		{
			if ($Userlist == $User)
			{
			$userbeenfound = TRUE;
			$id = $index;
			}
		}
		if ($userbeenfound)
		{
			unset($this->users[$id]); 
			$this->users = array_values($this->users);
		}
		return $userbeenfound;
	}
	function ReplaceUser($oldUser,$newUser)
	{
		$userbeenfound = FALSE;
		foreach ($this->users as $index => $Userlist) 
		{
			if ($Userlist == $oldUser)
			{
			$userbeenfound = TRUE;
			$id = $index;
			}
		}
		if ($userbeenfound)
		{
			$this->users[$id] = $newUser;
		}
		return $userbeenfound;
	}
}
// gobal var for muting the Bot
$SRB2MS_mute = FALSE;
//$SRB2MSRSS10 = fetch_rss( 'http://srb2.servegame.org/RSS1.0.php' );
// thing to hold RSS stuff
$rssdatabase = &new RSSDataBase_Class();
// PHP error reporting
//error_reporting(E_ERROR);
// the Bot is the Class SRB2MS_BOT
$bot = &new SRB2MS_BOT();
// Create SmartIRC, and name it as <network>irc for use to, well, do irc stuff ^_^
$esperirc = &new Net_SmartIRC();
// To Debug or Not
//$esperirc->setDebug(SMARTIRC_DEBUG_ALL);
// Do connect to the Internet
$esperirc->setUseSockets(TRUE);
// kep a list of all the users in the channel
$esperirc->setChannelSyncing(TRUE);
// Register the triggers with the class of SRB2MS_BOT
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!gamelist$', $bot, 'ListGamesChan');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!gamelist$', $bot, 'ListGamesPM');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!RSSgamelist$', $bot, 'RSSListGamesPM');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!gamelist0$', $bot, 'RSSListGames');
$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!gamelist$', $bot, 'RSSListGamesPM');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!gamelist$', $bot, 'RSSListGamesPM');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!addme$', $bot, 'RSSadduser');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!removeme$', $bot, 'RSSremoveuser');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!listusers$', $bot, 'ListUsersPM');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_NICKCHANGE, '', $bot, 'RSSupdateuser');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!checkme$', $bot, 'RSScheckuser');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!shutdown$', $bot, 'byebye');
$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!mute$', $bot, 'Mute');
$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!unmute$', $bot, 'Unmute');
$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!shutup$', $bot, 'Mute2');
$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!talktome$', $bot, 'Unmute2');
$esperirc->registerTimehandler(120000, $bot, 'RSSListUpdate');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_CHANNEL, '^!gamelist$', $bot, 'ListGamesChan');
$esperirc->registerActionhandler(SMARTIRC_TYPE_INVITE, '', $bot, 'BeenInvited');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!talk', $bot, 'Talk');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!action', $bot, 'Action');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!kick', $bot, 'Kick');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_QUERY, '^!ban', $bot, 'Ban');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_ALL, '', $bot, 'DebugPM');
//$esperirc->registerActionhandler(SMARTIRC_TYPE_KICK | SMARTIRC_TYPE_QUIT | SMARTIRC_TYPE_PART, '', $bot, 'DebugPM');
// If you fail once, try agian
$esperirc->setAutoRetry(TRUE);
// Connect to EsperNet
//$esperirc->connect('irc.esper.net', 6667);
$esperirc->connect('excalibur.esper.net', 6667);
// Reconnect as needed
$esperirc->setAutoReconnect(TRUE);
// Use the name SRB2MS, and set other stuff
$esperirc->login('SRB2MS', 'Net_SmartIRC Client '.SMARTIRC_VERSION.' (SRB2MS_Bot.php)', 0, 'Net_SmartIRC');
// Join #srb2fun
$esperirc->join(array('#srb2fun'));
// SRB2MS_Pass.php have "$esperirc->message(SMARTIRC_TYPE_QUERY, 'nickserv', 'identify <password>');"
include('./SRB2MS_Pass.php');
// Idle
$esperirc->listen();
// Rejoin?
$esperirc->reconnect();

?>
