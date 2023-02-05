
flash:
	source $(IDF_EXPORT_ENVS) && idf.py flash

erase:
	source $(IDF_EXPORT_ENVS) && idf.py erase-flash

