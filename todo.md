# Things to do

- when saving the config, do checks to make sure rubbish values arent being inputted.  For example: making sure strings arent blank.
- enable ap mode for self configuration
- add warning led
- add OTA
- add scheduled reboots
- make handleUpload support api uploads
- make LCD sizing dynamic
- for LCD, rather than returning html embeddedin json, make full json
- make pushover functionality better
- when doing http client, check return code
- add multiple temp probes
- resize json doc handling sizes
- make time formatting consistent and to a normal standard
- rename syslogSend to logSend
- add /fullstatus
- if a setting is changed and requires a reboot, notify user
- clean up probescanner, comments and other junk
- check the json doc size

# Done
- after scanning for probes, the saving of the newly discovered ones should be reconciled against any currently loaded ones
- figure out how to handle the 5v power needs
- fix boot time
- after calling /set redirect back to / with a message displayed in status
- be able to see what the lcd display is currently showing
