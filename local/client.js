var net = require('net');
var url = require("url");
var path = require("path");
var fs = require("fs");

var host = process.argv[2] || '127.0.0.1';
var port = process.argv[3] || 5052;
var filename = process.argv[4] || null;

var client = new net.Socket();

client.on('close', function ()
{
	console.log('[local]: Connection closed');
});

client.on('error', function(err)
{
	console.log('[local]: Connection could not be established');
	//console.log(err);
});

client.on('data', function(data)
{
	process.stdout.write(data.toString());
});

client.connect(port, host, function()
{
	console.log('[local]: Connected ' + host + ':' + port);

	if(filename == null)
		console.log('[local]: No file provided -> debug mode (out shell)');
	else
	{
		console.log('[local]: File (' + filename + ') provided -> sending mode');
		fs.exists(filename, function(exists)
		{
			if(!exists)
			{
				console.log('[local]: File ' + filename + ' doesn\'t exist');
				client.close();
			}

			if (fs.statSync(filename).isDirectory())
			{
				console.log('[local]: ' + filename + ' is a dictionary');
				client.end();
			}

			var readStream = fs.createReadStream(filename);

			readStream.on('open', function ()
			{
				console.log('[local]: Sending file');
				readStream.pipe(client);
			});

			readStream.on('end', function()
			{
				console.log('[local]: Send file');
				client.end();
			});

			readStream.on('error', function(err)
			{
				console.log('[local]: Connection could not be established');
				console.log(err);
				client.end();
			});
		});
	}
});

console.log('[local]: Trying to connect to ' + host + ':' + port);
