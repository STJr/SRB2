SET SQL_MODE = "NO_AUTO_VALUE_ON_ZERO";
SET AUTOCOMMIT = 0;
START TRANSACTION;
SET time_zone = "+00:00";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8mb4 */;

--
-- Database: `srb2ms`
--

CREATE DATABASE IF NOT EXISTS `srb2_ms` DEFAULT CHARACTER SET utf8 COLLATE utf8_general_ci;
USE `srb2_ms`;


-- --------------------------------------------------------

--
-- Table structure for table `ms_bans`
--

CREATE TABLE `ms_bans` (
  `bid` int(11) DEFAULT NULL,
  `ipstart` int(10) unsigned DEFAULT NULL,
  `ipend` int(10) unsigned DEFAULT NULL,
  `full_endtime` int(11) DEFAULT NULL,
  `permanent` tinyint(1) DEFAULT NULL,
  `hostonly` tinyint(1) DEFAULT NULL,
  `reason` text COLLATE utf8mb4_unicode_ci
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `ms_rooms`
--

CREATE TABLE `ms_rooms` (
  `room_id` int(11) NOT NULL,
  `title` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `motd` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `visible` tinyint(1) NOT NULL,
  `order` int(11) NOT NULL,
  `private` tinyint(1) NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- Dumping data for table `ms_rooms`
--

INSERT INTO `ms_rooms` (`room_id`, `title`, `motd`, `visible`, `order`, `private`) VALUES
(10, 'Example', 'Example Room', 1, 0, 0),
(0, 'All', 'View all of the servers currently being hosted.', 1, 1, 1);

-- --------------------------------------------------------

--
-- Table structure for table `ms_servers`
--

CREATE TABLE `ms_servers` (
  `sid` int(11) primary key AUTO_INCREMENT,
  `name` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `ip` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `port` int(11) NOT NULL DEFAULT 5029,
  `version` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `timestamp` int(11) NOT NULL DEFAULT 0,
  `room` int(11) NOT NULL DEFAULT 0,
  `key` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `room_override` int(11) NOT NULL DEFAULT 0,
  `upnow` tinyint(1) NOT NULL DEFAULT 1,
  `permanent` tinyint(1) NOT NULL DEFAULT 0,
  `delisted` tinyint(1) NOT NULL DEFAULT 0,
  `sticky` int(11) NOT NULL DEFAULT 0
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

-- --------------------------------------------------------

--
-- Table structure for table `ms_versions`
--

CREATE TABLE `ms_versions` (
  `mod_id` int(10) unsigned primary key AUTO_INCREMENT,
  `mod_version` int(10) unsigned NOT NULL DEFAULT 1,
  `mod_vstring` varchar(45) NOT NULL DEFAULT 'v1.0',
  `mod_codebase` int(10) unsigned NOT NULL DEFAULT 205,
  `mod_name` varchar(255) NOT NULL DEFAULT 'Default MOD Name',
  `mod_url` text NOT NULL 
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;

--
-- Dumping data for table `ms_versions`
--

INSERT INTO `ms_versions` (`mod_id`, `mod_version`, `mod_vstring`, `mod_codebase`, `mod_name`, `mod_url`) VALUES
(18, 42, 'v2.2.2', 205, 'SRB2 2.2', 'SRB2.org');

-- --------------------------------------------------------

--
-- Table structure for table `user`
--

CREATE TABLE `user` (
  `userid` int(11) NOT NULL,
  `username` text COLLATE utf8mb4_unicode_ci NOT NULL,
  `live_authkey` blob NOT NULL,
  `live_publickey` blob NOT NULL,
  `salt` blob NOT NULL,
  `password` blob NOT NULL
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_unicode_ci;
COMMIT;

/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;