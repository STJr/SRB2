<?php
	header("Content-type: text/plain; charset=ISO-8859-1");
        $host_addr = "Alam_GBC's box (srb2.servegame.org : 28900)";
        $fd = fsockopen("srb2.servegame.org", 28900, $errno, $errstr, 5);
	if ($fd)
	{
		stream_set_timeout ($fd, 5);
		echo "SRB2 Master Server Status\nCurrent host: $host_addr\n";
		$buff = "000012360000";
		fwrite($fd, $buff);
		while (1)
		{
			$content=fgets($fd, 13); // skip 13 first bytes
			$content=fgets($fd, 1024);
			echo "$content";
			if (feof($fd)) break;
		}
		fclose($fd);
	}
	else
	{
		echo "The master server is not running";
	}
?>
