#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <alsa/asoundlib.h>

void print_dB(long dB)
{
	printf("%li.%02lidB", dB / 100, (dB < 0 ? -dB : dB) % 100);
}

int convert_prange(int val, int min, int max)
{
	int range = max - min;
	int tmp;

	if (range == 0)
		return 0;
	val -= min;
	tmp = rint((double)val/(double)range * 100);
	return tmp;
}

const char *get_percent(int val, int min, int max)
{
	static char str[32];
	int p;
	
	p = convert_prange(val, min, max);
	sprintf(str, "%i [%i%%]", val, p);
	return str;
}

int show_selem(snd_mixer_t *handle, snd_mixer_selem_id_t *id, const char *space)
{
	snd_mixer_selem_channel_id_t chn;
	long pmin = 0, pmax = 0;
	long cmin = 0, cmax = 0;
	long pvol, cvol;
	int psw, csw;
	int pmono, cmono, mono_ok = 0;
	long db;
	snd_mixer_elem_t *elem;
	
	elem = snd_mixer_find_selem(handle, id);
	if (!elem) {
		printf("Mixer %s simple element not found", "default");
		return -ENOENT;
	}

	printf("%sCapabilities:", space);
	if (snd_mixer_selem_has_common_volume(elem)) {
		printf(" volume");
		if (snd_mixer_selem_has_playback_volume_joined(elem))
			printf(" volume-joined");
	} else {
		if (snd_mixer_selem_has_playback_volume(elem)) {
			printf(" pvolume");
			if (snd_mixer_selem_has_playback_volume_joined(elem))
				printf(" pvolume-joined");
		}
		if (snd_mixer_selem_has_capture_volume(elem)) {
			printf(" cvolume");
			if (snd_mixer_selem_has_capture_volume_joined(elem))
				printf(" cvolume-joined");
		}
	}
	if (snd_mixer_selem_has_common_switch(elem)) {
		printf(" switch");
		if (snd_mixer_selem_has_playback_switch_joined(elem))
			printf(" switch-joined");
	} else {
		if (snd_mixer_selem_has_playback_switch(elem)) {
			printf(" pswitch");
			if (snd_mixer_selem_has_playback_switch_joined(elem))
				printf(" pswitch-joined");
		}
		if (snd_mixer_selem_has_capture_switch(elem)) {
			printf(" cswitch");
			if (snd_mixer_selem_has_capture_switch_joined(elem))
				printf(" cswitch-joined");
			if (snd_mixer_selem_has_capture_switch_exclusive(elem))
				printf(" cswitch-exclusive");
		}
	}
	if (snd_mixer_selem_is_enum_playback(elem)) {
		printf(" penum");
	} else if (snd_mixer_selem_is_enum_capture(elem)) {
		printf(" cenum");
	} else if (snd_mixer_selem_is_enumerated(elem)) {
		printf(" enum");
	}
	printf("\n");
	if (snd_mixer_selem_is_enumerated(elem)) {
		int i, items;
		unsigned int idx;
		char itemname[40];
		items = snd_mixer_selem_get_enum_items(elem);
		printf("  Items:");
		for (i = 0; i < items; i++) {
			snd_mixer_selem_get_enum_item_name(elem, i, sizeof(itemname) - 1, itemname);
			printf(" '%s'", itemname);
		}
		printf("\n");
		for (i = 0; !snd_mixer_selem_get_enum_item(elem, i, &idx); i++) {
			snd_mixer_selem_get_enum_item_name(elem, idx, sizeof(itemname) - 1, itemname);
			printf("  Item%d: '%s'\n", i, itemname);
		}
		return 0; /* no more thing to do */
	}
	if (snd_mixer_selem_has_capture_switch_exclusive(elem))
		printf("%sCapture exclusive group: %i\n", space,
			   snd_mixer_selem_get_capture_group(elem));
	if (snd_mixer_selem_has_playback_volume(elem) ||
		snd_mixer_selem_has_playback_switch(elem)) {
		printf("%sPlayback channels:", space);
		if (snd_mixer_selem_is_playback_mono(elem)) {
			printf(" Mono");
		} else {
			int first = 1;
			for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++){
				if (!snd_mixer_selem_has_playback_channel(elem, chn))
					continue;
				if (!first)
					printf(" -");
				printf(" %s", snd_mixer_selem_channel_name(chn));
				first = 0;
			}
		}
		printf("\n");
	}
	if (snd_mixer_selem_has_capture_volume(elem) ||
		snd_mixer_selem_has_capture_switch(elem)) {
		printf("%sCapture channels:", space);
		if (snd_mixer_selem_is_capture_mono(elem)) {
			printf(" Mono");
		} else {
			int first = 1;
			for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++){
				if (!snd_mixer_selem_has_capture_channel(elem, chn))
					continue;
				if (!first)
					printf(" -");
				printf(" %s", snd_mixer_selem_channel_name(chn));
				first = 0;
			}
		}
		printf("\n");
	}
	if (snd_mixer_selem_has_playback_volume(elem) ||
		snd_mixer_selem_has_capture_volume(elem)) {
		printf("%sLimits:", space);
		if (snd_mixer_selem_has_common_volume(elem)) {
			snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
			snd_mixer_selem_get_capture_volume_range(elem, &cmin, &cmax);
			
			snd_mixer_selem_set_playback_volume(elem, 0, pmax);
			snd_mixer_selem_set_playback_volume(elem, 1, pmax);
			snd_mixer_selem_set_capture_volume(elem, 0, cmax);
			snd_mixer_selem_set_capture_volume(elem, 1, cmax);
			
			printf(" pppppp %li - %li", pmin, pmax);
			printf(" cccccc %li - %li", pmin, pmax);
		} else {
			if (snd_mixer_selem_has_playback_volume(elem)) {
				snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
				snd_mixer_selem_set_playback_volume(elem, 0, pmax);
				snd_mixer_selem_set_playback_volume(elem, 1, pmax);
				printf(" Playback %li - %li", pmin, pmax);
			}
			if (snd_mixer_selem_has_capture_volume(elem)) {
				snd_mixer_selem_get_capture_volume_range(elem, &cmin, &cmax);
				snd_mixer_selem_set_capture_volume(elem, 0, pmax);
				snd_mixer_selem_set_capture_volume(elem, 1, pmax);
				printf(" Capture %li - %li", cmin, cmax);
			}
		}
		printf("\n");
	}
	pmono = snd_mixer_selem_has_playback_channel(elem, SND_MIXER_SCHN_MONO) &&
			(snd_mixer_selem_is_playback_mono(elem) || 
		 (!snd_mixer_selem_has_playback_volume(elem) &&
		  !snd_mixer_selem_has_playback_switch(elem)));
	cmono = snd_mixer_selem_has_capture_channel(elem, SND_MIXER_SCHN_MONO) &&
			(snd_mixer_selem_is_capture_mono(elem) || 
		 (!snd_mixer_selem_has_capture_volume(elem) &&
		  !snd_mixer_selem_has_capture_switch(elem)));
#if 0
	printf("pmono = %i, cmono = %i (%i, %i, %i, %i)\n", pmono, cmono,
			snd_mixer_selem_has_capture_channel(elem, SND_MIXER_SCHN_MONO),
			snd_mixer_selem_is_capture_mono(elem),
			snd_mixer_selem_has_capture_volume(elem),
			snd_mixer_selem_has_capture_switch(elem));
#endif
	if (pmono || cmono) {
		if (!mono_ok) {
			printf("%s%s:", space, "Mono");
			mono_ok = 1;
		}
		if (snd_mixer_selem_has_common_volume(elem)) {
			snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &pvol);
			printf(" %s", get_percent(pvol, pmin, pmax));
			if (!snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_MONO, &db)) {
				printf(" [");
				print_dB(db);
				printf("]");
			}
		}
		if (snd_mixer_selem_has_common_switch(elem)) {
			snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &psw);
			printf(" [%s]", psw ? "on" : "off");
		}
	}
	if (pmono && snd_mixer_selem_has_playback_channel(elem, SND_MIXER_SCHN_MONO)) {
		int title = 0;
		if (!mono_ok) {
			printf("%s%s:", space, "Mono");
			mono_ok = 1;
		}
		if (!snd_mixer_selem_has_common_volume(elem)) {
			if (snd_mixer_selem_has_playback_volume(elem)) {
				printf(" Playback");
				title = 1;
				snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_MONO, &pvol);
				printf(" %s", get_percent(pvol, pmin, pmax));
				if (!snd_mixer_selem_get_playback_dB(elem, SND_MIXER_SCHN_MONO, &db)) {
					printf(" [");
					print_dB(db);
					printf("]");
				}
			}
		}
		if (!snd_mixer_selem_has_common_switch(elem)) {
			if (snd_mixer_selem_has_playback_switch(elem)) {
				if (!title)
					printf(" Playback");
				snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_MONO, &psw);
				printf(" [%s]", psw ? "on" : "off");
			}
		}
	}
	if (cmono && snd_mixer_selem_has_capture_channel(elem, SND_MIXER_SCHN_MONO)) {
		int title = 0;
		if (!mono_ok) {
			printf("%s%s:", space, "Mono");
			mono_ok = 1;
		}
		if (!snd_mixer_selem_has_common_volume(elem)) {
			if (snd_mixer_selem_has_capture_volume(elem)) {
				printf(" Capture");
				title = 1;
				snd_mixer_selem_get_capture_volume(elem, SND_MIXER_SCHN_MONO, &cvol);
				printf(" %s", get_percent(cvol, cmin, cmax));
				if (!snd_mixer_selem_get_capture_dB(elem, SND_MIXER_SCHN_MONO, &db)) {
					printf(" [");
					print_dB(db);
					printf("]");
				}
			}
		}
		if (!snd_mixer_selem_has_common_switch(elem)) {
			if (snd_mixer_selem_has_capture_switch(elem)) {
				if (!title)
					printf(" Capture");
				snd_mixer_selem_get_capture_switch(elem, SND_MIXER_SCHN_MONO, &csw);
				printf(" [%s]", csw ? "on" : "off");
			}
		}
	}
	if (pmono || cmono)
		printf("\n");
	if (!pmono || !cmono) {
		for (chn = 0; chn <= SND_MIXER_SCHN_LAST; chn++) {
			if ((pmono || !snd_mixer_selem_has_playback_channel(elem, chn)) &&
				(cmono || !snd_mixer_selem_has_capture_channel(elem, chn)))
				continue;
			printf("%s%s:", space, snd_mixer_selem_channel_name(chn));
			if (!pmono && !cmono && snd_mixer_selem_has_common_volume(elem)) {
				snd_mixer_selem_get_playback_volume(elem, chn, &pvol);
				printf(" %s", get_percent(pvol, pmin, pmax));
				if (!snd_mixer_selem_get_playback_dB(elem, chn, &db)) {
					printf(" [");
					print_dB(db);
					printf("]");
				}
			}
			if (!pmono && !cmono && snd_mixer_selem_has_common_switch(elem)) {
				snd_mixer_selem_get_playback_switch(elem, chn, &psw);
				printf(" [%s]", psw ? "on" : "off");
			}
			if (!pmono && snd_mixer_selem_has_playback_channel(elem, chn)) {
				int title = 0;
				if (!snd_mixer_selem_has_common_volume(elem)) {
					if (snd_mixer_selem_has_playback_volume(elem)) {
						printf(" Playback");
						title = 1;
						snd_mixer_selem_get_playback_volume(elem, chn, &pvol);
						printf(" %s", get_percent(pvol, pmin, pmax));
						if (!snd_mixer_selem_get_playback_dB(elem, chn, &db)) {
							printf(" [");
							print_dB(db);
							printf("]");
						}
					}
				}
				if (!snd_mixer_selem_has_common_switch(elem)) {
					if (snd_mixer_selem_has_playback_switch(elem)) {
						if (!title)
							printf(" Playback");
						snd_mixer_selem_get_playback_switch(elem, chn, &psw);
						printf(" [%s]", psw ? "on" : "off");
					}
				}
			}
			if (!cmono && snd_mixer_selem_has_capture_channel(elem, chn)) {
				int title = 0;
				if (!snd_mixer_selem_has_common_volume(elem)) {
					if (snd_mixer_selem_has_capture_volume(elem)) {
						printf(" Capture");
						title = 1;
						snd_mixer_selem_get_capture_volume(elem, chn, &cvol);
						printf(" %s", get_percent(cvol, cmin, cmax));
						if (!snd_mixer_selem_get_capture_dB(elem, chn, &db)) {
							printf(" [");
							print_dB(db);
							printf("]");
						}
					}
				}
				if (!snd_mixer_selem_has_common_switch(elem)) {
					if (snd_mixer_selem_has_capture_switch(elem)) {
						if (!title)
							printf(" Capture");
						snd_mixer_selem_get_capture_switch(elem, chn, &csw);
						printf(" [%s]", csw ? "on" : "off");
					}
				}
			}
			printf("\n");
		}
	}
	return 0;
}

int snd_set_volume(int val)
{
	snd_mixer_t *mixer;
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

	snd_mixer_open(&mixer, 0);
	snd_mixer_attach(mixer, "dmixer");
	snd_mixer_selem_register(mixer, NULL, NULL);
	snd_mixer_load(mixer);

#if 0
	// "Headphone"	
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_index(sid, 0);
	snd_mixer_selem_id_set_name(sid, "Headphone Playback Volume");
	
	elem = snd_mixer_find_selem(mixer, sid);
	if(elem) {
		printf("=== Master\n");
		show_selem(mixer, sid, "    ");
		snd_mixer_selem_set_playback_switch_all(elem, 0);
		printf("==========\n");
		show_selem(mixer, sid, "    ");
		snd_mixer_selem_set_playback_volume_all(elem, 5);
		printf("==========\n");
		show_selem(mixer, sid, "    ");
	} else {
		printf("========== not found sid\n");
	
	}
#endif
	
#if 1
	elem = snd_mixer_first_elem(mixer);
	for( ; elem; elem = snd_mixer_elem_next(elem)) {
		if(snd_mixer_elem_get_type(elem) != SND_MIXER_ELEM_SIMPLE || 
			snd_mixer_selem_has_playback_volume(elem) == 0)
			continue;
		const char *pname = snd_mixer_selem_get_name(elem);
		printf("==%s\n", pname);
		
		if(strncmp(pname, "Mic", 3) != 0) continue;
		
		// snd_mixer_selem_set_playback_switch_all(elem, 0);
		
		snd_mixer_selem_id_alloca(&sid);
		snd_mixer_selem_id_set_index(sid, 0);
		snd_mixer_selem_id_set_name(sid, pname);
		
		show_selem(mixer, sid, "    ");
		
		// snd_mixer_selem_id_free(sid);
	}
#endif

#if 0
	elem = snd_mixer_find_selem(mixer, sid);
	if (elem != NULL) {
		long int vol, minvol, maxvol;
		snd_mixer_selem_get_playback_volume(elem, 1, &vol);
		snd_mixer_selem_get_playback_volume_range(elem, &minvol, &maxvol);
		printf("volume = %d, and range [%d,%d]\n", vol, minvol, maxvol);
		
		snd_mixer_selem_set_playback_switch_all(elem, 0);
		snd_mixer_selem_set_playback_volume_all(elem, val);
		
		snd_mixer_selem_get_playback_volume(elem, 1, &vol);
		
		snd_mixer_selem_set_playback_volume_range(elem, 0, 100);
		snd_mixer_selem_get_playback_volume_range(elem, &minvol, &maxvol);
		printf("volume = %d, and range [%d,%d]\n", vol, minvol, maxvol);
	}
#endif
	snd_mixer_close(mixer);
	return 0;
}

int main(int argc, char *argv[])
{
	unsigned int val;
	int rc;
	char *buffer;
	snd_pcm_t *handle;
	snd_pcm_hw_params_t *params;
	//snd_pcm_uframes_t frames;
	
	//printf("======== PCM capture ========\n");

	/* Open PCM device for capture. */
	rc = snd_pcm_open(&handle, "default", SND_PCM_STREAM_CAPTURE, 0);
	if (rc < 0) {
		printf("unable to open pcm device: %s\n", snd_strerror(rc));
		exit(1);
	}

	snd_pcm_hw_params_alloca(&params);
	snd_pcm_hw_params_any(handle, params);
	snd_pcm_hw_params_set_access(handle, params, SND_PCM_ACCESS_RW_INTERLEAVED);
	snd_pcm_hw_params_set_format(handle, params, SND_PCM_FORMAT_S16_LE);
	snd_pcm_hw_params_set_channels(handle, params, 2);
	
	// val = 11025 - 1;
	// val = 22050 - 1;
	val = 44100 - 1;
	// val = 48000 - 1;
	// val = 64000 - 1;
	// val = 96000 - 1;
	
	snd_pcm_hw_params_set_rate_near(handle, params, &val, 0);
	
	unsigned period_time = 0;
	unsigned buffer_time = 0;
	snd_pcm_hw_params_get_buffer_time_max(params, &buffer_time, 0);
	if (buffer_time > 500000) buffer_time = 500000;
	period_time = buffer_time >> 2;
	snd_pcm_hw_params_set_period_time_near(handle, params, &period_time, 0);
	snd_pcm_hw_params_set_buffer_time_near(handle, params, &buffer_time, 0);
	
	/* Set period size to 32 frames. */
	//frames = FRAMES;
	//snd_pcm_hw_params_set_period_size_near(handle, params, &frames, &dir);

	/* Write the parameters to the driver */
	rc = snd_pcm_hw_params(handle, params);
	if (rc < 0) {
		printf("unable to set hw parameters: %s\n", snd_strerror(rc));
		snd_pcm_close(handle);
		exit(1);
	}
	
	snd_set_volume(2);
	
	buffer = (char *) malloc(1024);

	/* Use a buffer large enough to hold one period */
	//snd_pcm_hw_params_get_period_size(params, &frames, &dir);
	//snd_pcm_hw_params_get_period_time(params, &val, &dir);
	//printf("======== val = %d\tdir=%d\n", val, dir);
	
	do {
		rc = snd_pcm_readi(handle, buffer, 256);
		if (rc == -EPIPE) {
			/* EPIPE means overrun */
			printf("overrun occurred\n");
			snd_pcm_prepare(handle);
		} else if (rc < 0) {
			printf("error from read: %s\n", snd_strerror(rc));
		} else if (rc != 256) {
			printf("short read, read %d frames\n", rc);
		}
		
		rc = write(1, buffer, 1024);
		if (rc != 1024)
			printf("short write: wrote %d bytes\n", rc);
		
	} while (1);
	
	snd_pcm_drain(handle);
	snd_pcm_close(handle);
	
	free(buffer);
	
	return 0;
}










