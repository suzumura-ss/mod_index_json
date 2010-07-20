mod_index_json.la: mod_index_json.slo
	$(SH_LINK) -rpath $(libexecdir) -module -avoid-version  mod_index_json.lo
DISTCLEAN_TARGETS = modules.mk
shared =  mod_index_json.la
