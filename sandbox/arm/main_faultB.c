#include <stdio.h>
#include <stdint.h>
#include <assert.h>

uint8_t get_threshold1(uint64_t syndrome_weight);
uint8_t get_threshold3(uint64_t syndrome_weight);
uint8_t get_threshold5(uint64_t syndrome_weight);

#define R_BITS_LEVEL1 12323
#define R_BITS_LEVEL3 24659
#define R_BITS_LEVEL5 40973


uint8_t expected_get_threshold1(uint64_t w) {
    if (w < 3223) return 255;
    if (w < 3367) return 36;
    if (w < 3510) return 37;
    if (w < 3654) return 38;
    if (w < 3797) return 39;
    if (w < 3940) return 40;
    if (w < 4084) return 41;
    if (w < 4227) return 42;
    if (w < 4371) return 43;
    if (w < 4514) return 44;
    if (w < 4658) return 45;
    if (w < 4801) return 46;
    if (w < 4944) return 47;
    if (w < 5088) return 48;
    if (w < 5231) return 49;
    if (w < 5375) return 50;
    if (w < 5518) return 51;
    if (w < 5662) return 52;
    if (w < 5805) return 53;
    if (w < 5948) return 54;
    if (w < 6092) return 55;
    if (w < 6235) return 56;
    if (w < 6379) return 57;
    if (w < 6522) return 58;
    if (w < 6666) return 59;
    if (w < 6809) return 60;
    if (w < 6952) return 61;
    if (w < 7096) return 62;
    if (w < 7239) return 63;
    if (w < 7383) return 64;
    if (w < 7526) return 65;
    if (w < 7670) return 66;
    if (w < 7813) return 67;
    if (w < 7956) return 68;
    if (w < 8100) return 69;
    if (w < 8243) return 70;
    if (w < 8387) return 71;
    if (w < 8530) return 72;
    if (w < 8674) return 73;
    if (w < 8817) return 74;
    if (w < 8960) return 75;
    if (w < 9104) return 76;
    if (w < 9247) return 77;
    if (w < 9391) return 78;
    if (w < 9534) return 79;
    if (w < 9678) return 80;
    if (w < 9821) return 81;
    if (w < 9964) return 82;
    if (w < 10108) return 83;
    if (w < 10251) return 84;
    if (w < 10395) return 85;
    if (w < 10538) return 86;
    if (w < 10681) return 87;
    if (w < 10825) return 88;
    if (w < 10968) return 89;
    if (w < 11112) return 90;
    if (w < 11255) return 91;
    if (w < 11399) return 92;
    if (w < 11542) return 93;
    if (w < 11685) return 94;
    if (w < 11829) return 95;
    if (w < 11972) return 96;
    if (w < 12116) return 97;
    if (w < 12259) return 98;
    return 99;
}


uint8_t expected_get_threshold3(uint64_t w) {
    if (w < 6979) return 255;
    if (w < 7169) return 52;
    if (w < 7359) return 53;
    if (w < 7549) return 54;
    if (w < 7739) return 55;
    if (w < 7929) return 56;
    if (w < 8118) return 57;
    if (w < 8308) return 58;
    if (w < 8498) return 59;
    if (w < 8688) return 60;
    if (w < 8878) return 61;
    if (w < 9068) return 62;
    if (w < 9258) return 63;
    if (w < 9448) return 64;
    if (w < 9638) return 65;
    if (w < 9828) return 66;
    if (w < 10018) return 67;
    if (w < 10208) return 68;
    if (w < 10398) return 69;
    if (w < 10588) return 70;
    if (w < 10778) return 71;
    if (w < 10967) return 72;
    if (w < 11157) return 73;
    if (w < 11347) return 74;
    if (w < 11537) return 75;
    if (w < 11727) return 76;
    if (w < 11917) return 77;
    if (w < 12107) return 78;
    if (w < 12297) return 79;
    if (w < 12487) return 80;
    if (w < 12677) return 81;
    if (w < 12867) return 82;
    if (w < 13057) return 83;
    if (w < 13247) return 84;
    if (w < 13437) return 85;
    if (w < 13627) return 86;
    if (w < 13816) return 87;
    if (w < 14006) return 88;
    if (w < 14196) return 89;
    if (w < 14386) return 90;
    if (w < 14576) return 91;
    if (w < 14766) return 92;
    if (w < 14956) return 93;
    if (w < 15146) return 94;
    if (w < 15336) return 95;
    if (w < 15526) return 96;
    if (w < 15716) return 97;
    if (w < 15906) return 98;
    if (w < 16096) return 99;
    if (w < 16286) return 100;
    if (w < 16476) return 101;
    if (w < 16665) return 102;
    if (w < 16855) return 103;
    if (w < 17045) return 104;
    if (w < 17235) return 105;
    if (w < 17425) return 106;
    if (w < 17615) return 107;
    if (w < 17805) return 108;
    if (w < 17995) return 109;
    if (w < 18185) return 110;
    if (w < 18375) return 111;
    if (w < 18565) return 112;
    if (w < 18755) return 113;
    if (w < 18945) return 114;
    if (w < 19135) return 115;
    if (w < 19325) return 116;
    if (w < 19514) return 117;
    if (w < 19704) return 118;
    if (w < 19894) return 119;
    if (w < 20084) return 120;
    if (w < 20274) return 121;
    if (w < 20464) return 122;
    if (w < 20654) return 123;
    if (w < 20844) return 124;
    if (w < 21034) return 125;
    if (w < 21224) return 126;
    if (w < 21414) return 127;
    if (w < 21604) return 128;
    if (w < 21794) return 129;
    if (w < 21984) return 130;
    if (w < 22174) return 131;
    if (w < 22364) return 132;
    if (w < 22553) return 133;
    if (w < 22743) return 134;
    if (w < 22933) return 135;
    if (w < 23123) return 136;
    if (w < 23313) return 137;
    if (w < 23503) return 138;
    if (w < 23693) return 139;
    if (w < 23883) return 140;
    if (w < 24073) return 141;
    if (w < 24263) return 142;
    if (w < 24453) return 143;
    if (w < 24643) return 144;
    return 145;
}

uint8_t expected_get_threshold5(uint64_t w) {
    if (w < 12707) return 255;
    if (w < 12956) return 69;
    if (w < 13205) return 70;
    if (w < 13453) return 71;
    if (w < 13702) return 72;
    if (w < 13950) return 73;
    if (w < 14199) return 74;
    if (w < 14447) return 75;
    if (w < 14696) return 76;
    if (w < 14944) return 77;
    if (w < 15193) return 78;
    if (w < 15442) return 79;
    if (w < 15690) return 80;
    if (w < 15939) return 81;
    if (w < 16187) return 82;
    if (w < 16436) return 83;
    if (w < 16684) return 84;
    if (w < 16933) return 85;
    if (w < 17182) return 86;
    if (w < 17430) return 87;
    if (w < 17679) return 88;
    if (w < 17927) return 89;
    if (w < 18176) return 90;
    if (w < 18424) return 91;
    if (w < 18673) return 92;
    if (w < 18922) return 93;
    if (w < 19170) return 94;
    if (w < 19419) return 95;
    if (w < 19667) return 96;
    if (w < 19916) return 97;
    if (w < 20164) return 98;
    if (w < 20413) return 99;
    if (w < 20661) return 100;
    if (w < 20910) return 101;
    if (w < 21159) return 102;
    if (w < 21407) return 103;
    if (w < 21656) return 104;
    if (w < 21904) return 105;
    if (w < 22153) return 106;
    if (w < 22401) return 107;
    if (w < 22650) return 108;
    if (w < 22899) return 109;
    if (w < 23147) return 110;
    if (w < 23396) return 111;
    if (w < 23644) return 112;
    if (w < 23893) return 113;
    if (w < 24141) return 114;
    if (w < 24390) return 115;
    if (w < 24638) return 116;
    if (w < 24887) return 117;
    if (w < 25136) return 118;
    if (w < 25384) return 119;
    if (w < 25633) return 120;
    if (w < 25881) return 121;
    if (w < 26130) return 122;
    if (w < 26378) return 123;
    if (w < 26627) return 124;
    if (w < 26876) return 125;
    if (w < 27124) return 126;
    if (w < 27373) return 127;
    if (w < 27621) return 128;
    if (w < 27870) return 129;
    if (w < 28118) return 130;
    if (w < 28367) return 131;
    if (w < 28615) return 132;
    if (w < 28864) return 133;
    if (w < 29113) return 134;
    if (w < 29361) return 135;
    if (w < 29610) return 136;
    if (w < 29858) return 137;
    if (w < 30107) return 138;
    if (w < 30355) return 139;
    if (w < 30604) return 140;
    if (w < 30853) return 141;
    if (w < 31101) return 142;
    if (w < 31350) return 143;
    if (w < 31598) return 144;
    if (w < 31847) return 145;
    if (w < 32095) return 146;
    if (w < 32344) return 147;
    if (w < 32592) return 148;
    if (w < 32841) return 149;
    if (w < 33090) return 150;
    if (w < 33338) return 151;
    if (w < 33587) return 152;
    if (w < 33835) return 153;
    if (w < 34084) return 154;
    if (w < 34332) return 155;
    if (w < 34581) return 156;
    if (w < 34830) return 157;
    if (w < 35078) return 158;
    if (w < 35327) return 159;
    if (w < 35575) return 160;
    if (w < 35824) return 161;
    if (w < 36072) return 162;
    if (w < 36321) return 163;
    if (w < 36570) return 164;
    if (w < 36818) return 165;
    if (w < 37067) return 166;
    if (w < 37315) return 167;
    if (w < 37564) return 168;
    if (w < 37812) return 169;
    if (w < 38061) return 170;
    if (w < 38309) return 171;
    if (w < 38558) return 172;
    if (w < 38807) return 173;
    if (w < 39055) return 174;
    if (w < 39304) return 175;
    if (w < 39552) return 176;
    if (w < 39801) return 177;
    if (w < 40049) return 178;
    if (w < 40298) return 179;
    if (w < 40547) return 180;
    if (w < 40795) return 181;
    return 182;
}

int test(void) {
    printf("x = [");
    // for (uint64_t w = 0; w < 10000; w += 100) {
    for (uint64_t w = 0; w <= R_BITS_LEVEL5; w += 1) {
        uint8_t thr = get_threshold5(w);
        // assert(thr == expected_get_threshold5(w));
        // printf("get_threshold(%llu) = %u\n", w, thr);
        printf("%u,", thr);
        fflush(stdout);
    }
    printf("]\n");

    return 0;
}

int main(void) {

    for (uint64_t w = 0; w <= R_BITS_LEVEL1; w += 1) {
        uint8_t thr = get_threshold1(w);
        assert(thr == expected_get_threshold1(w));
    }
    printf("Level 1 (FAULT B) expected get_threshold OK\n");


    for (uint64_t w = 0; w <= R_BITS_LEVEL3; w += 1) {
        uint8_t thr = get_threshold3(w);
        assert(thr == expected_get_threshold3(w));
    }
    printf("Level 3 (FAULT B) expected get_threshold OK\n");

    for (uint64_t w = 0; w <= R_BITS_LEVEL5; w += 1) {
        uint8_t thr = get_threshold5(w);
        assert(thr == expected_get_threshold5(w));
    }
    printf("Level 5 (FAULT B) expected get_threshold OK\n");

    return 0;
}