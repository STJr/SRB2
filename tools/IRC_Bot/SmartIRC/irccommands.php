<?php
/**
 * $Id: irccommands.php,v 1.1.2.1 2003/10/10 20:20:58 meebey Exp $
 * $Revision: 1.1.2.1 $
 * $Author: meebey $
 * $Date: 2003/10/10 20:20:58 $
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

class Net_SmartIRC_irccommands extends Net_SmartIRC_base
{
    /**
     * sends a new message
     *
     * Sends a message to a channel or user.
     *
     * @see DOCUMENTATION
     * @param integer $type specifies the type, like QUERY/ACTION or CTCP see 'Message Types'
     * @param string $destination can be a user or channel
     * @param string $message the message
     * @return boolean
     * @access public
     */
    function message($type, $destination, $message, $priority = SMARTIRC_MEDIUM)
    {
        switch ($type) {
            case SMARTIRC_TYPE_CHANNEL:
            case SMARTIRC_TYPE_QUERY:
                $this->_send('PRIVMSG '.$destination.' :'.$message, $priority);
            break;
            case SMARTIRC_TYPE_ACTION:
                $this->_send('PRIVMSG '.$destination.' :'.chr(1).'ACTION '.$message.chr(1), $priority);
            break;
            case SMARTIRC_TYPE_NOTICE:
                $this->_send('NOTICE '.$destination.' :'.$message, $priority);
            break;
            case SMARTIRC_TYPE_CTCP: // backwards compatibilty
            case SMARTIRC_TYPE_CTCP_REPLY:
                $this->_send('NOTICE '.$destination.' :'.chr(1).$message.chr(1), $priority);
            break;
            case SMARTIRC_TYPE_CTCP_REQUEST:
                $this->_send('PRIVMSG '.$destination.' :'.chr(1).$message.chr(1), $priority);
            break;
            default:
                return false;
        }
            
        return true;
    }
    
    /**
     * returns an object reference to the specified channel
     *
     * If the channel does not exist (because not joint) false will be returned.
     *
     * @param string $channelname
     * @return object reference to the channel object
     * @access public
     */
    function &channel($channelname)
    {
        if (isset($this->_channels[strtolower($channelname)])) {
            return $this->_channels[strtolower($channelname)];
        } else {
            return false;
        }
    }
    
    // <IRC methods>
    /**
     * Joins one or more IRC channels with an optional key.
     *
     * @param mixed $channelarray
     * @param string $key 
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function join($channelarray, $key = null, $priority = SMARTIRC_MEDIUM)
    {
        if (!is_array($channelarray)) {
            $channelarray = array($channelarray);
        }
        
        $channellist = implode(',', $channelarray);
        
        if ($key !== null) {
            $this->_send('JOIN '.$channellist.' '.$key, $priority);
        } else {
            $this->_send('JOIN '.$channellist, $priority);
        }
    }
    
    /**
     * parts from one or more IRC channels with an optional reason
     *
     * @param mixed $channelarray 
     * @param string $reason
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function part($channelarray, $reason = null, $priority = SMARTIRC_MEDIUM)
    {
        if (!is_array($channelarray)) {
            $channelarray = array($channelarray);
        }
        
        $channellist = implode(',', $channelarray);
        
        if ($reason !== null) {
            $this->_send('PART '.$channellist.' :'.$reason, $priority);
        } else {
            $this->_send('PART '.$channellist, $priority);
        }
    }
    
    /**
     * Kicks one or more user from an IRC channel with an optional reason.
     *
     * @param string $channel
     * @param mixed $nicknamearray
     * @param string $reason
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function kick($channel, $nicknamearray, $reason = null, $priority = SMARTIRC_MEDIUM)
    {
        if (!is_array($nicknamearray)) {
            $nicknamearray = array($nicknamearray);
        }
        
        $nicknamelist = implode(',', $nicknamearray);
        
        if ($reason !== null) {
            $this->_send('KICK '.$channel.' '.$nicknamelist.' :'.$reason, $priority);
        } else {
            $this->_send('KICK '.$channel.' '.$nicknamelist, $priority);
        }
    }
    
    /**
     * gets a list of one ore more channels
     *
     * Requests a full channellist if $channelarray is not given.
     * (use it with care, usualy its a looooong list)
     *
     * @param mixed $channelarray
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function getList($channelarray = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($channelarray !== null) {
            if (!is_array($channelarray)) {
                $channelarray = array($channelarray);
            }
            
            $channellist = implode(',', $channelarray);
            $this->_send('LIST '.$channellist, $priority);
        } else {
            $this->_send('LIST', $priority);
        }
    }

    /**
     * requests all nicknames of one or more channels
     *
     * The requested nickname list also includes op and voice state
     *
     * @param mixed $channelarray
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function names($channelarray = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($channelarray !== null) {
            if (!is_array($channelarray)) {
                $channelarray = array($channelarray);
            }
            
            $channellist = implode(',', $channelarray);
            $this->_send('NAMES '.$channellist, $priority);
        } else {
            $this->_send('NAMES', $priority);
        }
    }
    
    /**
     * sets a new topic of a channel
     *
     * @param string $channel
     * @param string $newtopic
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function setTopic($channel, $newtopic, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('TOPIC '.$channel.' :'.$newtopic, $priority);
    }
    
    /**
     * gets the topic of a channel
     *
     * @param string $channel
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function getTopic($channel, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('TOPIC '.$channel, $priority);
    }
    
    /**
     * sets or gets the mode of an user or channel
     *
     * Changes/requests the mode of the given target.
     *
     * @param string $target the target, can be an user (only yourself) or a channel
     * @param string $newmode the new mode like +mt
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function mode($target, $newmode = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($newmode !== null) {
            $this->_send('MODE '.$target.' '.$newmode, $priority);
        } else {
            $this->_send('MODE '.$target, $priority);
        }
    }
    
    /**
     * ops an user in the given channel
     *
     * @param string $channel
     * @param string $nickname
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function op($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '+o '.$nickname, $priority);
    }
    
    /**
     * deops an user in the given channel
     *
     * @param string $channel
     * @param string $nickname
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function deop($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '-o '.$nickname, $priority);
    }
    
    /**
     * voice a user in the given channel
     *
     * @param string $channel
     * @param string $nickname
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function voice($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '+v '.$nickname, $priority);
    }
    
    /**
     * devoice a user in the given channel
     *
     * @param string $channel
     * @param string $nickname
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function devoice($channel, $nickname, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '-v '.$nickname, $priority);
    }
    
    /**
     * bans a hostmask for the given channel or requests the current banlist
     *
     * The banlist will be requested if no hostmask is specified
     *
     * @param string $channel
     * @param string $hostmask
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function ban($channel, $hostmask = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($hostmask !== null) {
            $this->mode($channel, '+b '.$hostmask, $priority);
        } else {
            $this->mode($channel, 'b', $priority);
        }
    }
    
    /**
     * unbans a hostmask on the given channel
     *
     * @param string $channel
     * @param string $hostmask
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function unban($channel, $hostmask, $priority = SMARTIRC_MEDIUM)
    {
        $this->mode($channel, '-b '.$hostmask, $priority);
    }
    
    /**
     * invites a user to the specified channel
     *
     * @param string $nickname
     * @param string $channel
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function invite($nickname, $channel, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('INVITE '.$nickname.' '.$channel, $priority);
    }
    
    /**
     * changes the own nickname
     *
     * Trys to set a new nickname, nickcollisions are handled.
     *
     * @param string $newnick
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function changeNick($newnick, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('NICK '.$newnick, $priority);
        $this->_nick = $newnick;
    }
    
    /**
     * requests a 'WHO' from the specified target
     *
     * @param string $target
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function who($target, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('WHO '.$target, $priority);
    }
    
    /**
     * requests a 'WHOIS' from the specified target
     *
     * @param string $target
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function whois($target, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('WHOIS '.$target, $priority);
    }
    
    /**
     * requests a 'WHOWAS' from the specified target
     * (if he left the IRC network)
     *
     * @param string $target
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function whowas($target, $priority = SMARTIRC_MEDIUM)
    {
        $this->_send('WHOWAS '.$target, $priority);
    }
    
    /**
     * sends QUIT to IRC server and disconnects
     *
     * @param string $quitmessage optional quitmessage
     * @param integer $priority message priority, default is SMARTIRC_MEDIUM
     * @return void
     * @access public
     */
    function quit($quitmessage = null, $priority = SMARTIRC_MEDIUM)
    {
        if ($quitmessage !== null) {
            $this->_send('QUIT :'.$quitmessage, $priority);
        } else {
            $this->_send('QUIT', $priority);
        }
    }
}
?>