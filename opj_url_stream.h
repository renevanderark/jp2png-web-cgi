/**
    opj_url_stream.h handlers for random access url decoding using libcurl
    Copyright (C) 2013  René van der Ark

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


struct opj_url_stream_data {
	OPJ_UINT64 position;
	OPJ_UINT64 size;
	const char *url;
};

OPJ_UINT64 get_url_data_length(const char * address);
OPJ_SIZE_T opj_read_from_url (void * p_buffer, OPJ_SIZE_T p_nb_bytes, struct opj_url_stream_data * p_url);
OPJ_BOOL opj_seek_from_url (OPJ_OFF_T p_nb_bytes, struct opj_url_stream_data * p_url);
OPJ_OFF_T opj_skip_from_url (OPJ_OFF_T p_nb_bytes, struct opj_url_stream_data * p_url);


