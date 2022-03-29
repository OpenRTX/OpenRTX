#include <interfaces/cps_io.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

static FILE *cps_file = NULL;
const char *default_author = "Codeplug author.";
const char *default_descr = "Codeplug description.";

/**
 * Internal: read and validate codeplug header
 *
 * @param header: pointer to the header struct to be populated
 * @return 0 on success, -1 on failure
 */
bool _readHeader(cps_header_t *header) {
    fseek(cps_file, 0L, SEEK_SET);
    fread(header, sizeof(cps_header_t), 1, cps_file);
    // Validate magic number
    if(header->magic != CPS_MAGIC)
        return -1;
    // Validate version number
    if(((header->version_number & 0xff00) >> 8) != CPS_VERSION_MAJOR ||
        (header->version_number & 0x00ff) > CPS_VERSION_MINOR)
        return -1;
    return 0;
}

/**
 * Internal: push down data at a given offset by a given amount
 *
 * @param offset: offset at which to start to push down data
 * @param amount: amount of free space to be created
 * @return 0 on success, -1 on failure
 */
int _pushDown(uint32_t offset, uint32_t amount) {


}

int cps_open(char *cps_name) {
    if (!cps_name)
        cps_name = "default.rtxc";
    cps_file = fopen(cps_name, "r+");
    if (!cps_file)
        return -1;
    return 0;
}

void cps_close() {
    fclose(cps_file);
}

int cps_create(char *cps_name) {
    // Clear or create cps file
    FILE *new_cps = NULL;
    if (!cps_name)
        cps_name = "default.rtxc";
    new_cps = fopen(cps_name, "w");
    if (!new_cps)
        return -1;
    // Write new header
    cps_header_t header = { 0 };
    header.magic = CPS_MAGIC;
    header.version_number = CPS_VERSION_MAJOR << 8 | CPS_VERSION_MINOR;
    strncpy(header.author, default_author, 17);
    strncpy(header.descr, default_descr, 23);
    // TODO: Implement unix timestamp in Miosix
    header.timestamp = time(NULL);
    header.ct_count = 0;
    header.ch_count = 0;
    header.b_count = 0;
    fwrite(&header, sizeof(cps_header_t), 1, new_cps);
    fclose(new_cps);
    return 0;
}

int cps_readContactData(contact_t *contact, uint16_t pos) {
    cps_header_t header = { 0 };
    if (!_readHeader(&header))
        return -1;
    if (pos >= header.ct_count)
        return -1;
    fseek(cps_file, pos * sizeof(contact_t), SEEK_CUR);
    fread(contact, sizeof(contact_t), 1, cps_file);
    return 0;
}

int cps_readChannelData(channel_t *channel, uint16_t pos) {
    cps_header_t header = { 0 };
    if (!_readHeader(&header))
        return -1;
    if (pos >= header.ch_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          pos * sizeof(channel_t),
          SEEK_CUR);
    fread(channel, sizeof(channel_t), 1, cps_file);
    return 0;
}

int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos) {
    cps_header_t header = { 0 };
    if (!_readHeader(&header))
        return -1;
    if (pos >= header.b_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    fseek(cps_file, header.b_count - pos * sizeof(uint32_t) + offset, SEEK_CUR);
    fread(b_header, sizeof(bankHdr_t), 1, cps_file);
    return 0;
}

uint32_t cps_readBankData(uint16_t bank_pos, uint16_t ch_pos) {
    cps_header_t header = { 0 };
    if (!_readHeader(&header))
        return -1;
    if (bank_pos >= header.b_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          bank_pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    fseek(cps_file, header.b_count - bank_pos * sizeof(uint32_t) + offset, SEEK_CUR);
    bankHdr_t b_header = { 0 };
    fread(&b_header, sizeof(bankHdr_t), 1, cps_file);
    if (ch_pos >= b_header.ch_count)
        return -1;
    fseek(cps_file, ch_pos * sizeof(uint32_t), SEEK_CUR);
    uint32_t ch_index = 0;
    fread(&ch_index, sizeof(uint32_t), 1, cps_file);
    return ch_index;
}

int cps_writeContactData(contact_t contact, uint16_t pos) {

}
