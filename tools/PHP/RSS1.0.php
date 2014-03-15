<?php
header("Content-type: text/xml; charset=ISO-8859-1");
echo '<?xml version="1.0" encoding="ISO-8859-1"?>';
echo "\n";
echo '<rdf:RDF';
echo '  xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"';
echo "\n";
echo '  xmlns="http://purl.org/rss/1.0/"';
echo "\n";
echo '  xmlns:dc="http://purl.org/dc/elements/1.1/"';
echo "\n";
echo '  xmlns:srb2ms="http://srb2.servegame.org/SRB2MS/elements/"';
echo "\n";
echo '>';
echo "\n";
echo "\n";
echo '  <channel rdf:about="http://srb2.servegame.org/">';
echo "\n";
echo '    <title>SRB2 Master Server RSS Feed</title>';
echo "\n";
echo '    <link>http://srb2.servegame.org/</link>';
echo "\n";
echo '    <description>Playing around with RSS</description>';
echo "\n";
//echo '    <language>en-us</language>';
//echo "\n";
        $fd = fsockopen("srb2.servegame.org", 28900, $errno, $errstr, 5);
//        $fd = 0;
	if ($fd)
	{
		$buff = "000012400000";
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
		echo '<items>';
		echo '<rdf:Seq>';
		echo '<rdf:li rdf:resource="http://srb2.servegame.org/" />';
		echo '</rdf:Seq>';
		echo '</items>';
		echo '</channel>';
		echo "\n";
		echo '<item rdf:about="http://srb2.servegame.org/"> ';
		echo '<title>No master server</title><dc:description>The master server is not running</dc:description></item>';
	}
?>
</rdf:RDF>