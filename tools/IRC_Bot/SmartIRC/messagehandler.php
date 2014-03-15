<?php
/**
 * $Id: messagehandler.php,v 1.25.2.5 2004/09/23 23:24:22 meebey Exp $
 * $Revision: 1.25.2.5 $
 * $Author: meebey $
 * $Date: 2004/09/23 23:24:22 $
 *
 * Copyright (c) 2002-2003 Mirco "MEEBEY" Bauer <mail@meebey.net> <http://www.meebey.net>
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

class Net_SmartIRC_messagehandler extends Net_SmartIRC_irccommands
{
    /* misc */
    function _event_ping(&$ircdata)
    {
        $this->_pong(substr($ircdata->rawmessage, 5));
    }
    
    function _event_error(&$ircdata)
    {
        if ($this->_autoretry == true) {
            $this->reconnect();
        } else {
            $this->disconnect(true);
        }
    }
    
    function _event_join(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            if ($this->_nick == $ircdata->nick) {
                $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: joining channel: '.$ircdata->channel, __FILE__, __LINE__);
                $channel = &new Net_SmartIRC_channel();
                $channel->name = $ircdata->channel;
                $this->_channels[strtolower($channel->name)] = &$channel;
                
                $this->who($channel->name);
                $this->mode($channel->name);
                $this->ban($channel->name);
            }
            
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: '.$ircdata->nick.' joins channel: '.$ircdata->channel, __FILE__, __LINE__);
            $channel = &$this->_channels[strtolower($ircdata->channel)];
            $user = &new Net_SmartIRC_channeluser();
            $user->nick = $ircdata->nick;
            $user->ident = $ircdata->ident;
            $user->host = $ircdata->host;
            
            $this->_adduser($channel, $user);
            $this->who($user->nick);
        }
    }
    
    function _event_part(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $this->_removeuser($ircdata);
        }
    }
    
    function _event_kick(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $this->_removeuser($ircdata);
        }
    }
    
    function _event_quit(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $this->_removeuser($ircdata);
        }
    }
    
    function _event_nick(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $newnick = substr($ircdata->rawmessageex[2], 1);
            $lowerednewnick = strtolower($newnick);
            $lowerednick = strtolower($ircdata->nick);
            
            foreach ($this->_channels as $channelkey => $channelvalue) {
                // loop through all channels
                $channel = &$this->_channels[$channelkey];
                foreach ($channel->users as $userkey => $uservalue) {
                    // loop through all user in this channel
                    
                    if ($ircdata->nick == $uservalue->nick) {
                        // found him
                        // time for updating the object and his nickname
                        $channel->users[$lowerednewnick] = $channel->users[$lowerednick];
                        $channel->users[$lowerednewnick]->nick = $newnick;
                        
                        if ($lowerednewnick != $lowerednick) {
                            unset($channel->users[$lowerednick]);
                        }
                        
                        // he was maybe op or voice, update comming
                        if (isset($channel->ops[$ircdata->nick])) {
                            $channel->ops[$newnick] = $channel->ops[$ircdata->nick];
                            unset($channel->ops[$ircdata->nick]);
                        }
                        if (isset($channel->voices[$ircdata->nick])) {
                            $channel->voices[$newnick] = $channel->voices[$ircdata->nick];
                            unset($channel->voices[$ircdata->nick]);
                        }
                        
                        break;
                    }
                }
            }
        }
    }
    
    function _event_mode(&$ircdata)
    {
        // check if its own usermode
        if ($ircdata->rawmessageex[2] == $this->_nick) {
            $this->_usermode = substr($ircdata->rawmessageex[3], 1);
        } else if ($this->_channelsyncing == true) {
            // it's not, and we do channel syching
            $channel = &$this->_channels[strtolower($ircdata->channel)];
            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: updating channel mode for: '.$channel->name, __FILE__, __LINE__);
            $mode = $ircdata->rawmessageex[3];
            $parameters = array_slice($ircdata->rawmessageex, 4);
            
            $add = false;
            $remove = false;
            $channelmode = '';
            $modelength = strlen($mode);
            for ($i = 0; $i < $modelength; $i++) {
                switch($mode[$i]) {
                    case '-':
                        $remove = true;
                        $add = false;
                    break;
                    case '+':
                        $add = true;
                        $remove = false;
                    break;
                    // user modes
                    case 'o':
                        $nick = array_shift($parameters);
                        $lowerednick = strtolower($nick);
                        if ($add) {
                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: adding op: '.$nick.' to channel: '.$channel->name, __FILE__, __LINE__);
                            $channel->ops[$nick] = true;
                            $channel->users[$lowerednick]->op = true;
                        }
                        if ($remove) {
                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing op: '.$nick.' to channel: '.$channel->name, __FILE__, __LINE__);
                            unset($channel->ops[$nick]);
                            $channel->users[$lowerednick]->op = false;
                        }
                    break;
                    case 'v':
                        $nick = array_shift($parameters);
                        $lowerednick = strtolower($nick);
                        if ($add) {
                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: adding voice: '.$nick.' to channel: '.$channel->name, __FILE__, __LINE__);
                            $channel->voices[$nick] = true;
                            $channel->users[$lowerednick]->voice = true;
                        }
                        if ($remove) {
                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing voice: '.$nick.' to channel: '.$channel->name, __FILE__, __LINE__);
                            unset($channel->voices[$nick]);
                            $channel->users[$lowerednick]->voice = false;
                        }
                    break;
                    case 'k':
                        $key = array_shift($parameters);
                        if ($add) {
                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: stored channel key for: '.$channel->name, __FILE__, __LINE__);
                            $channel->key = $key;
                        }
                        if ($remove) {
                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removed channel key for: '.$channel->name, __FILE__, __LINE__);
                            $channel->key = '';
                        }
                    break;
                    default:
                        // channel modes
                        if ($mode[$i] == 'b') {
                            $hostmask = array_shift($parameters);
                            if ($add) {
                                $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: adding ban: '.$hostmask.' for: '.$channel->name, __FILE__, __LINE__);
                                $channel->bans[$hostmask] = true;
                            }
                            if ($remove) {
                                $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: removing ban: '.$hostmask.' for: '.$channel->name, __FILE__, __LINE__);
                                unset($channel->bans[$hostmask]);
                            }
                        } else {
                            $this->log(SMARTIRC_DEBUG_CHANNELSYNCING, 'DEBUG_CHANNELSYNCING: storing unknown channelmode ('.$mode.') in channel->mode for: '.$channel->name, __FILE__, __LINE__);
                            if ($add) {
                                $channel->mode .= $mode[$i];
                            }
                            if ($remove) {
                                $channel->mode = str_replace($mode[$i], '', $channel->mode);
                            }
                        }
                }
            }
        }
    }
    
    function _event_topic(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $channel = &$this->_channels[strtolower($ircdata->rawmessageex[2])];
            $channel->topic = $ircdata->message;
        }
    }
    
    function _event_privmsg(&$ircdata)
    {
        if ($ircdata->type == SMARTIRC_TYPE_CTCP) {
            // substr must be 1,4 because of \001 in CTCP messages
            if (substr($ircdata->message, 1, 4) == 'PING') {
                $this->message(SMARTIRC_TYPE_CTCP, $ircdata->nick, 'PING '.substr($ircdata->message, 5, -1));
            } elseif (substr($ircdata->message, 1, 7) == 'VERSION') {
                if (!empty($this->_ctcpversion)) {
                    $versionstring = $this->_ctcpversion.' | using '.SMARTIRC_VERSIONSTRING;
                } else {
                    $versionstring = SMARTIRC_VERSIONSTRING;
                }
                
                $this->message(SMARTIRC_TYPE_CTCP, $ircdata->nick, 'VERSION '.$versionstring);
            }
        }
    }
    
    /* rpl_ */
    function _event_rpl_welcome(&$ircdata)
    {
        $this->_loggedin = true;
        $this->log(SMARTIRC_DEBUG_CONNECTION, 'DEBUG_CONNECTION: logged in', __FILE__, __LINE__);
        
        // updating our nickname, that we got (maybe cutted...)
        $this->_nick = $ircdata->rawmessageex[2];
    }
    
    function _event_rpl_motdstart(&$ircdata)
    {
        $this->_motd[] = $ircdata->message;
    }
    
    function _event_rpl_motd(&$ircdata)
    {
        $this->_motd[] = $ircdata->message;
    }
    
    function _event_rpl_endofmotd(&$ircdata)
    {
        $this->_motd[] = $ircdata->message;
    }
    
    function _event_rpl_umodeis(&$ircdata)
    {
        $this->_usermode = $ircdata->message;
    }
    
    function _event_rpl_channelmodeis(&$ircdata) {
        if ($this->_channelsyncing == true && $this->isJoined($ircdata->channel)) {
            $mode = $ircdata->rawmessageex[4];
            $parameters = array_slice($ircdata->rawmessageex, 5);
            
            $ircdata->rawmessageex = array( 0 => '',
                                            1 => '',
                                            2 => '',
                                            3 => $mode);
            
            foreach ($parameters as $value) {
                $ircdata->rawmessageex[] = $value;
            }
            
            // let _mode() handle the received mode
            $this->_event_mode($ircdata);
        }
    }
    
    function _event_rpl_whoreply(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $nick = $ircdata->rawmessageex[7];
            if ($ircdata->channel == '*') {
                // we got who info without channel info, so we need to search the user
                // on all channels and update him
                foreach ($this->_channels as $channel) {
                    if ($this->isJoined($channel->name, $nick)) {
                        $ircdata->channel = $channel->name;
                        $this->_event_rpl_whoreply($ircdata);
                    }
                }
            } else {
                if (!$this->isJoined($ircdata->channel, $nick)) {
                    return;
                }
                                
                $channel = &$this->_channels[strtolower($ircdata->channel)];
                
                $user = &new Net_SmartIRC_channeluser();
                $user->ident = $ircdata->rawmessageex[4];
                $user->host = $ircdata->rawmessageex[5];
                $user->server = $ircdata->rawmessageex[6];
                $user->nick = $ircdata->rawmessageex[7];
                
                $user->op = false;
                $user->voice = false;
                $user->ircop = false;
                
                $usermode = $ircdata->rawmessageex[8];
                $usermodelength = strlen($usermode);
                for ($i = 0; $i < $usermodelength; $i++) {
                    switch ($usermode[$i]) {
                        case 'H':
                            $user->away = false;
                        break;
                        case 'G':
                            $user->away = true;
                        break;
                        case '@':
                            $user->op = true;
                        break;
                        case '+':
                            $user->voice = true;
                        break;
                        case '*':
                            $user->ircop = true;
                        break;
                    }
                }
                 
                $user->hopcount = substr($ircdata->rawmessageex[9], 1);
                $user->realname = implode(array_slice($ircdata->rawmessageex, 10), ' ');
                
                $this->_adduser($channel, $user);
            }
        }
    }
    
    function _event_rpl_namreply(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $channel = &$this->_channels[strtolower($ircdata->channel)];
            
            $userarray = explode(' ', rtrim($ircdata->message));
            $userarraycount = count($userarray);
            for ($i = 0; $i < $userarraycount; $i++) {
                $user = &new Net_SmartIRC_channeluser();
                
                $usermode = substr($userarray[$i], 0, 1);
                switch ($usermode) {
                    case '@':
                        $user->op = true;
                        $user->nick = substr($userarray[$i], 1);
                    break;
                    case '+':
                        $user->voice = true;
                        $user->nick = substr($userarray[$i], 1);
                    break;
                    default:
                        $user->nick = $userarray[$i];
                }
                
                $this->_adduser($channel, $user);
            }
        }
    }
    
    function _event_rpl_banlist(&$ircdata)
    {
        if ($this->_channelsyncing == true && $this->isJoined($ircdata->channel)) {
            $channel = &$this->_channels[strtolower($ircdata->channel)];
            $hostmask = $ircdata->rawmessageex[4];
            $channel->bans[$hostmask] = true;
        }
    }
    
    function _event_rpl_topic(&$ircdata)
    {
        if ($this->_channelsyncing == true) {
            $channel = &$this->_channels[strtolower($ircdata->channel)];
            $topic = substr(implode(array_slice($ircdata->rawmessageex, 4), ' '), 1);
            $channel->topic = $topic;
        }
    }
    
    /* err_ */
    function _event_err_nicknameinuse(&$ircdata)
    {
        $this->_nicknameinuse();
    }
}
?>