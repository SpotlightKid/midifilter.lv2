MFD_FILTER(noterange)

#ifdef MX_TTF

	mflt:noterange
	TTF_DEFAULTDEF("MIDI Key-Range Filter")
	, TTF_IPORT(0, "channelf", "Filter Channel",  0.0, 16.0,  0.0, PORTENUMZ("Any"))
	, TTF_IPORT(1, "lower", "Lowest Note",  0.0, 127.0,  0.0, lv2:portProperty lv2:integer; units:unit units:midiNote)
	, TTF_IPORT(2, "upper", "Highest Note",  0.0, 127.0,  127.0, lv2:portProperty lv2:integer; units:unit units:midiNote)
	, TTF_IPORT(3, "mode", "Operation Mode",  0.0, 3.0,  1.0,
			lv2:portProperty lv2:integer; lv2:portProperty lv2:enumeration;
			lv2:scalePoint [ rdfs:label "Bypass"  ; rdf:value 0.0 ] ;
			lv2:scalePoint [ rdfs:label "Include Range"  ; rdf:value 1.0 ] ;
			lv2:scalePoint [ rdfs:label "Exclude Range"  ; rdf:value 2.0 ] ;
			)
	.

#elif defined MX_CODE

static void
filter_midi_noterange(MidiFilter* self,
		uint32_t tme,
		const uint8_t* const buffer,
		uint32_t size)
{
	const int mode = RAIL(floor(*self->cfg[3]),0, 3);
	const uint8_t chs = midi_limit_chn(floor(*self->cfg[0]) -1);
	const uint8_t chn = buffer[0] & 0x0f;
	uint8_t mst = buffer[0] & 0xf0;

	if (size != 3
			|| !(mst == MIDI_NOTEON || mst == MIDI_NOTEOFF)
			|| !(floor(*self->cfg[0]) == 0 || chs == chn)
			|| mode == 0
		 )
	{
		forge_midimessage(self, tme, buffer, size);
		return;
	}

	const uint8_t low = midi_limit_val(floor(*self->cfg[1]));
	const uint8_t upp = midi_limit_val(floor(*self->cfg[2]));
	const uint8_t key = buffer[1] & 0x7f;
	const uint8_t vel = buffer[2] & 0x7f;

	if (mst == MIDI_NOTEON && vel ==0 ) {
		mst = MIDI_NOTEOFF;
	}

	switch(mst) {
		case MIDI_NOTEON:
			if ((key >= low && key <= upp) ^ (mode == 2)) {
				forge_midimessage(self, tme, buffer, size);
				self->memCM[chn][key] = vel;
			}
			break;
		case MIDI_NOTEOFF:
			if (self->memCM[chn][key] > 0) {
				forge_midimessage(self, tme, buffer, size);
			}
			self->memCM[chn][key] = 0;
			break;
	}
}

static void filter_preproc_noterange(MidiFilter* self) {
	if (   floor(self->lcfg[1]) == floor(*self->cfg[1])
			&& floor(self->lcfg[2]) == floor(*self->cfg[2])
			&& floor(self->lcfg[3]) == floor(*self->cfg[3])
			) return;

	int c,k;
	uint8_t buf[3];
	buf[2] = 0;

	const int mode = RAIL(floor(*self->cfg[3]),0, 3);
	const uint8_t low = midi_limit_val(floor(*self->cfg[1]));
	const uint8_t upp = midi_limit_val(floor(*self->cfg[2]));

	for (c=0; c < 16; ++c) {
		for (k=0; k < 127; ++k) {
			const uint8_t vel = self->memCM[c][k];
			if (vel == 0) continue;
			if (mode != 0 && (k >= low && k <= upp) ^ (mode == 2)) continue;
			buf[0] = MIDI_NOTEOFF | c;
			buf[1] = midi_limit_val(k + self->memCI[c][k]);
			forge_midimessage(self, 0, buf, 3);
			self->memCM[c][k] = 0;
		}
	}
}

static void filter_init_noterange(MidiFilter* self) {
	int c,k;
	for (c=0; c < 16; ++c) for (k=0; k < 127; ++k) {
		self->memCM[c][k] = 0;
	}
	self->preproc_fn = filter_preproc_noterange;
}

#endif
