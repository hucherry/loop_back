#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>

// ���Ĳ���
using namespace std;

const int MAXN = 222222;
const int BufferSize = 30;
char g_datetime[MAXN + 10][50];
char g_date[MAXN + 10][30];
char g_time[MAXN + 10][30];

double closeArray[MAXN + 10];
double openArray[MAXN + 10];
double highArray[MAXN + 10];
double lowArray[MAXN + 10];


int tradingNum ;
struct tradeNode{
    char entryTime[50];
    char exitTime[50];
    double entryPrice,exitPrice;
    double volume;
}tradingResult[MAXN + 10];
int pos;
int mp[MAXN + 10]; // marketpostion
int BarsSinceEntry; //

///////////////////////////////////////////////////////////////

double slippage = 1.0 , rate = 1.29/10000 , perSize = 10;

int lots = 1;
double zhisun_l = 0 , zhisun_s = 0;
double kai_up, kai_down;

double lips_N[MAXN+10] , teeth_N[MAXN+10] , croco_N[MAXN+10];
double lips[MAXN+10] , teeth[MAXN+10] , croco[MAXN+10];

int zuiArray[MAXN+10] , eyuArray[MAXN+10];
double high_since_entry[MAXN + 10] , low_since_entry[MAXN + 10];

int bufferCount = 0;
int n = 0;

int MinMove = 1 , PriceScale = 1;

//////  �˻���Ϣ��
int account_money = 40000;   // �˻���ʼ�ʽ�
vector<string> vecTradeDay;
vector<double> vecTradeTotalMoneyDay; // ÿ�յ����ʽ� = �ֲ� + �����ʽ�
vector<double> vecTradeRateDay;       // ÿ��
vector<string> vecTradeWeekDay;

/**
 * C++�жϽ��������ڼ�
 * ��ķ����ɭ���㹫ʽ
 * W= (d+2*m+3*(m+1)/5+y+y/4-y/100+y/400+1) mod 7
 * �ڹ�ʽ��d��ʾ�����е�������m��ʾ�·�����y��ʾ������
 * ע�⣺�ڹ�ʽ���и���������ʽ��ͬ�ĵط���
 * ��һ�ºͶ��¿�������һ���ʮ���º�ʮ���£����������2004-1-10����ɣ�2003-13-10�����빫ʽ���㡣
 **/
int CaculateWeekDay(int y, int m, int d)
{
    if(m==1||m==2) //��һ�ºͶ��»������һ���ʮ���º�������
    {
        m+=12;
        y--;
    }
    int Week=(d+2*m+3*(m+1)/5+y+y/4-y/100+y/400)%7;
    return Week;
}
int CaculateWeekDayS(char * s){
    int y,m,d;
    sscanf(s , "%d/%d/%d" , &y,&m,&d);
    return CaculateWeekDay(y,m,d);
}
//����xAverage
void xAverage(double *dstArr , const double *closeArray ,const int m, const int n){
    for(int i = 0;i < n;i++){
        if(i == 0)  dstArr[i] = closeArray[i];
        else  dstArr[i] = dstArr[i-1] + 2.0 * (closeArray[i] -  dstArr[i-1]) / (m + 1) ;
    }
}
//����ƽ�Ƶ�λ���㷨
void LeftMove(double *dstArr , const double *oriArray ,const int m, const int n){
    for(int i = 0;i < n;i++){
        dstArr[i] = oriArray[max(0, i-m)];
    }
}
//��ȡ���������
void loadFile(const char *filename , const char *start_date , const char *end_date){
    FILE *fp = NULL;
    if(fp = fopen(filename, "r")){
        n = 0;
        char tdate[50],ttime[50];
        double open,high,low,close,volume,openInterest;
        while(fscanf(fp,"%s %5s,%lf,%lf,%lf,%lf,%lf,%lf", tdate,ttime, &open,&high,&low,&close,&volume,&openInterest) != EOF){
            if(strcmp(start_date , tdate) <= 0 && strcmp(tdate , end_date) <= 0){
                sprintf(g_datetime[n] , "%s %s",tdate,ttime);
                strcpy(g_date[n] , tdate);   strcpy(g_time[n] , ttime);
                openArray[n] = open;  highArray[n] = high;  lowArray[n] = low; closeArray[n] = close;
                n = n + 1;
            }
        }
        fclose(fp);
        vecTradeDay.clear();  vecTradeWeekDay.clear();
        for(int i = 0;i < n;i++){
            if(i == 0){
                vecTradeDay.push_back(string(g_date[i]));
                if(CaculateWeekDayS(g_date[i]) == 4){
                    vecTradeWeekDay.push_back(g_date[i]);
                }
            }else{
                if( strcmp(vecTradeDay[vecTradeDay.size()-1].c_str() , g_date[i]) != 0){
                    vecTradeDay.push_back( string(g_date[i]));
                    if(CaculateWeekDayS(g_date[i]) == 4){
                        vecTradeWeekDay.push_back(g_date[i]);
                    }
                }
            }
        }
    }else{
        printf("open file %s Error!" , filename);
    }
}

// ���������кϷ����� ��Ҫ������Сһ�����д���
double legalTradePrice(double price, double volume){
    double legalPrice = price;
    if(volume > 0){
        legalPrice = (int)(price+0.999);
    }else{
        legalPrice = (int)price;
    }
    return legalPrice;
}

void Sell(int size , double price , int iBar){
    if(pos > 0){
        pos = pos - size;
        #ifdef debug
            printf("))pos %d sell %s %lf tradingNum %d\n", pos ,g_datetime[iBar] , price, tradingNum);
        #endif
        tradingResult[tradingNum].exitPrice = legalTradePrice(price, -1.0);
        strcpy(tradingResult[tradingNum].exitTime , g_datetime[iBar]);
        tradingNum += 1;
    }
}
void BuyToCover(int size , double price , int iBar){
    if(pos < 0){
        pos = pos + size;
        #ifdef debug
            printf("}}pos %d buytocover %s %lf tradingNum %d\n", pos ,g_datetime[iBar] , price, tradingNum);
        #endif
        tradingResult[tradingNum].exitPrice = legalTradePrice(price , 1.0 );
        strcpy(tradingResult[tradingNum].exitTime , g_datetime[iBar]);
        tradingNum += 1;
    }
}
void Buy(int size , double price , int iBar){
    if(pos < 0){
        BuyToCover( abs(pos) , price, iBar);
    }
    pos = pos + size;
    #ifdef debug
        printf("((pos %d buy %s %lf tradingNum %d\n", pos ,g_datetime[iBar] , price, tradingNum);
    #endif
    tradingResult[tradingNum].entryPrice = legalTradePrice(price, 1.0);
    tradingResult[tradingNum].volume = size;
    strcpy(tradingResult[tradingNum].entryTime , g_datetime[iBar]);
}
void SellShort(int size , double price , int iBar){
    if(pos > 0){
        Sell( fabs(pos) , price , iBar);
    }
    pos = pos - size;
    #ifdef debug
        printf("{{pos %d sellshort %s %lf tradingNum %d\n", pos ,g_datetime[iBar] , price, tradingNum);
    #endif
    tradingResult[tradingNum].entryPrice = legalTradePrice(price, -1.0);
    tradingResult[tradingNum].volume = -size;
    strcpy(tradingResult[tradingNum].entryTime , g_datetime[iBar]);
}

// ����barsEntry �������
void updateBarsEntry(int iBar){
    if(pos != 0){
        BarsSinceEntry += 1;
    }
    if(mp[max(0,iBar-1)] != 0 && 0 == mp[iBar]){
        BarsSinceEntry = 0;
    }
}

/**
 * ��¼ÿ�ν��׵�ӯ�����
 **/
struct TradingResultNode{
    double ini_account_money;           //�˻���ʼ�ʽ� F
    double rate_of_return;              //�껯������   F                      important
    int total_open_num;                 //�ܿ��ִ���   F                      important
    double win_rate;                    //ʤ��         F                      important
    double profit_factor;               //ӯ������   �������ܶ�/�����ܶ�  F   important
    double max_continue_loss;           //����������� F                      important
    double max_windrawal_rate;          //���س����� F                      important         ���س���ֵ / �ߵ�
    double sharpe;                      //���ձ���     F                      important         ÿ�ʽ���ƽ��ӯ���� / ÿ�ʽ��ױ�׼��

    int total_trade_days;               //�����ո���   F
    int total_trade_weeks;              //�����ܸ���   F
    double total_income;                //������ F
    double net_loss;                    //�ܿ��� F
    double net_profits;                 //��ӯ�� F

    int win_week_num;                   //ӯ������
    int loss_week_num;                  //��������
    int win_cover_num;                  //ƽ��ӯ������  F
    int loss_cover_num;                 //ƽ�ֿ������  F
    double loss_rate;                   //����          F
    double max_continue_win;            //�������ӯ��  F
    int max_continue_win_num;           //�����Ӯ����  F
    int max_continue_loss_num;          //�����������  F

    double ave_cover_income;            //ƽ��ƽ�־����� F      ƽ��ÿ�־�����
    double ave_cover_win;               //ƽ��ƽ��ӯ��   F      ƽ��ÿ��ӯ��
    double ave_cover_loss;              //ƽ��ƽ�ֿ���   F      ƽ��ÿ�ֿ���
    double ave_cover_win_loss_rate;     //ƽ��ƽ��ӯ���� F

    double max_single_win;              //��󵥱�ӯ��   F
    double max_single_loss;             //��󵥱ʿ���   F

    double max_windrawl_number;         //���س���ֵ     F
    char max_windraw_start_date[50];    //���س����俪ʼ F
    char max_windraw_end_date[50];      //���س�������� F
    double windrawl_safe_rate;          //�س���ȫϵ��     F    ������/���س���ֵ

    double total_transcaction_fee;      //�ܽ��׳ɱ�     F      �ܵ�(��֤�� + ������)
    double ave_transcation_fee;         //ƽ�����׳ɱ�   F
    double transcation_cover_rate;      //���׳ɱ������� F      ������/�ܽ��׳ɱ�

    double real_income_rate;            //ʵ��������     F      ������/��ʼ�ʽ�
    double ave_time_income_rate;        //ƽ��ÿ�������� F      ƽ��ÿ��������

    double variance;                    //������         W      ÿ�ʽ��ױ�׼��
    double kurtosis;                    //���           W
    double skewness;                    //ƫ��           W
    double sortinoRatio;                //����ŵ����     W      ÿ�ܵ�����ŵ

    double targetFunctionRankScore;     //Ŀ�꺯��������ʵ�ʷ��� F
    double targetFunctionPromScore;     //Ŀ�꺯��Prom��ʵ�ʷ��� F

    //prom Ŀ�꺯��
    double targetFunctionProm(){
        return (this->ave_cover_win * (this->win_cover_num - sqrt(this->win_cover_num * 1.0)) - this->ave_cover_loss * (this->loss_cover_num - sqrt(this->loss_cover_num * 1.0))) / this->ave_transcation_fee;
    }
    // Ŀ�꺯������
    double targetFunctionRank(){
        return this->rate_of_return * 0.3 + this->max_windrawal_rate * 0.2 + this->win_rate * 0.1 + this->kurtosis * this->skewness * 0.1 + this->profit_factor * 0.1 + this->sortinoRatio * 0.2;
    }
    //TradingResultNode(){}
    // ���׺����Ĺ��캯��
    TradingResultNode(const struct tradeNode * tradingResult , const int tradingNum , const double account_money, double slippage,double rate , const vector<string> &vecTradeDay , const vector<string> &vecTradeWeekDay){ //
        this->total_trade_days = vecTradeDay.size();
        this->total_trade_weeks = vecTradeWeekDay.size();
        this->ini_account_money = account_money;
        this->net_profits = 0; this->net_loss = 0;
        this->win_cover_num = 0; this->loss_cover_num = 0;
        this->total_open_num = tradingNum;
        this->max_single_win = 0.0;   this->max_single_loss = 0.0;
        this->max_continue_win = 0.0; this->max_continue_loss = 0.0;
        this->max_continue_win_num = 0; this->max_continue_loss_num = 0;
        this->total_income = 0.0;
        this->total_transcaction_fee = 0.0;

        double eps = 1e-5;
        double last_pnl = 0.0;                          //��һ�ʵ�pnl
        int continue_win_num = 0;                       //����׬Ǯ����
        int continue_loss_num = 0;                      //������Ǯ����
        double continue_income = 0.0;                   //����ӯ��
        double continue_loss = 0.0;                     //��������
        vector<double> v_pnl; v_pnl.clear();            //�洢������pnl
        vector<double> v_sum_pnl; v_sum_pnl.clear();    //�洢pnl��������
        for(int i = 0;i < tradingNum;i++){
            double turnover = (tradingResult[i].entryPrice + tradingResult[i].exitPrice) * perSize * fabs(tradingResult[i].volume);
            double commission = turnover * rate;
            double per_slippage = slippage * 2 * perSize * fabs(tradingResult[i].volume);
            double pnl = (tradingResult[i].exitPrice - tradingResult[i].entryPrice) * tradingResult[i].volume * perSize - commission - per_slippage;

            if(last_pnl * pnl > eps){     // ��Ϊ��������������
                if(pnl > 0){
                    continue_income += pnl;
                    continue_win_num += 1;
                }else{
                    continue_loss += pnl;
                    continue_loss_num += 1;
                }
            }else{  //��һ�ʵ�pnl�������ϴεĲ�һ��
                continue_win_num = continue_loss_num = 0;
                continue_income = continue_loss = 0;
                if(pnl > eps){
                    continue_win_num += 1;
                    continue_income += pnl;
                }
                if(-pnl > eps){
                    continue_loss_num += 1;
                    continue_loss += pnl;
                }
            }
            this->total_transcaction_fee += turnover + commission + per_slippage;
            this->total_income += pnl;
            if(tradingResult[i].volume > 0){
                this->net_profits += pnl;
                this->win_cover_num += 1;
            }
            if(tradingResult[i].volume < 0){
                this->net_loss += pnl;
                this->loss_cover_num += 1;
            }
            this->max_single_win = max(pnl , this->max_single_win);
            this->max_single_loss = min(pnl , this->max_single_loss);

            this->max_continue_win_num = max(this->max_continue_win_num , continue_win_num);
            this->max_continue_loss_num = max(this->max_continue_loss_num , continue_loss_num);
            this->max_continue_win = max(this->max_continue_win , continue_income);
            this->max_continue_loss = min(this->max_continue_loss , continue_loss);
            last_pnl = pnl;

            v_pnl.push_back(pnl);
            v_sum_pnl.push_back(this->total_income);
        }
        //this->total_income = this->net_profits + this->net_loss;

        this->ave_transcation_fee = this->total_transcaction_fee * 1.0 / this->total_open_num;
        this->transcation_cover_rate = this->total_income * 1.0 / this->total_transcaction_fee;
        this->win_rate = this->win_cover_num * 1.0 / this->total_open_num;
        this->loss_rate = this->loss_cover_num * 1.0 / this->total_open_num;
        this->profit_factor = fabs(this->net_profits / this->net_loss);
        this->ave_cover_income = this->total_income * 1.0 / this->total_open_num;
        this->ave_cover_win = this->net_profits * 1.0 / this->win_cover_num;
        this->ave_cover_loss = this->net_loss * 1.0 / this->loss_cover_num;
        this->ave_cover_win_loss_rate = this->ave_cover_win * 1.0 / this->ave_cover_loss;
        this->rate_of_return = this->total_income * 250.0 * 100.0 / (this->ini_account_money * this->total_trade_days);
        this->real_income_rate = this->total_income * 1.0 / this->ini_account_money;
        this->ave_time_income_rate = this->ave_cover_income * 1.0 / this->ini_account_money;

        double t_variance = 0.0;
        double t_kurtosis_fenzi = 0.0;
        double t_skewness = 0.0;
        double t_max_windrawl_number = 0.0;
        int t_max_windraw_start_date = 0, t_max_windraw_end_date = 0;
        double t_high_value = ini_account_money;
        double t_ddex = 0.0;

        double t_week_profit = 0.0;
        int tw = 0;
        this->win_week_num = 0 ;  this->loss_week_num = 0;
        for(int i = 0;i < tradingNum; i++){
            double pnl = v_pnl[i];
            t_variance += pow( (pnl - this->ave_cover_income),2.0);
            t_kurtosis_fenzi += pow(pnl - this->ave_cover_income,3.0);
            t_skewness += pow(pnl - this->ave_cover_income, 4.0);
            t_ddex += pow( min(0.0 , pnl) , 2);

            double sum_pnl1 = v_sum_pnl[i];
            for(int j = i+1;j < tradingNum;j++){
                double sum_pnl2 = v_sum_pnl[j];
                if( (sum_pnl2 - sum_pnl1) < t_max_windrawl_number){
                    t_max_windrawl_number = sum_pnl2 - sum_pnl1;
                    t_max_windraw_start_date = i;
                    t_max_windraw_end_date = j;
                    t_high_value = ini_account_money + sum_pnl1;
                }
            }

            if( strncmp( tradingResult[i].exitTime , vecTradeWeekDay[tw].c_str() , 10) <= 0 ){
                //printf("%s %s\n",tradingResult[i].exitTime , vecTradeWeekDay[tw].c_str());
                t_week_profit += pnl;
            }else{
                //printf("t_week_profit %lf\n",t_week_profit);
                if(t_week_profit > eps){
                    this->win_week_num += 1;
                }
                if(t_week_profit < eps){
                    this->loss_week_num += 1;
                }
                tw+=1;
                t_week_profit = pnl;
                //while( strncmp( tradingResult[i].exitTime , vecTradeWeekDay[tw].c_str() , 10) > 0) tw++;
            }
        }
        this->variance = sqrt(t_variance * 1.0 / (tradingNum - 1));
        this->sharpe = this->ave_cover_income * 1.0 * tradingNum / 250.0 / this->variance;
        this->kurtosis = t_kurtosis_fenzi * 1.0 / (tradingNum * pow(this->variance, 3.0));
        this->skewness = t_skewness * 1.0 / ( tradingNum * pow(this->variance , 4.0));
        this->sortinoRatio = sqrt(52.0) * this->rate_of_return * sqrt(tradingNum - 1) / sqrt(t_ddex);

        strcpy(this->max_windraw_start_date, tradingResult[t_max_windraw_start_date].exitTime);
        strcpy(this->max_windraw_end_date , tradingResult[t_max_windraw_end_date].exitTime);
        this->max_windrawl_number = t_max_windrawl_number;
        this->windrawl_safe_rate = fabs(this->total_income * 1.0 / this->max_windrawl_number);
        this->max_windrawal_rate = this->max_windrawl_number * 1.0 / t_high_value;

        this->targetFunctionRankScore = this->targetFunctionRank();
        this->targetFunctionPromScore = this->targetFunctionProm();
    }
    void showResult(){
        printf("�˻���ʼ�ʽ�:%lf\n",ini_account_money);
        printf("�껯����:%lf\n",rate_of_return);
        printf("�ܿ��ִ���:%d\n",total_open_num);
        printf("ʤ��:%lf\n",win_rate);
        printf("ӯ������:%lf\n",profit_factor);
        printf("�����������:%lf\n",max_continue_loss);
        printf("���س�����:%lf\n",max_windrawal_rate);
        printf("���ձ���:%lf\n",sharpe);
        printf("�����ո���:%d\n",total_trade_days);
        printf("�ܹ�������:%d\n",total_trade_weeks);
        printf("��ӯ��:%lf\n",net_profits);
        printf("�ܿ���:%lf\n",net_loss);
        printf("������:%lf\n",total_income);
        printf("ӯ������:%d\n",win_week_num);
        printf("��������:%d\n",loss_week_num);
        printf("����ӯ������:%d\n",win_cover_num);
        printf("ƽ�ֿ������:%d\n",loss_cover_num);
        printf("����:%lf\n",loss_rate);
        printf("�������ӯ��:%lf\n",max_continue_win);
        printf("�����Ӯ����:%d\n",max_continue_win_num);
        printf("�����������:%d\n",max_continue_loss_num);
        printf("ƽ��ƽ�־�����:%lf\n",ave_cover_income);
        printf("ƽ��ƽ��ӯ��:%lf\n",ave_cover_win);
        printf("ƽ��ƽ�ֿ���:%lf\n",ave_cover_loss);
        printf("ƽ��ƽ��ӯ����:%lf\n",ave_cover_win_loss_rate);
        printf("��󵥱�ӯ��:%lf\n",max_single_win);
        printf("��󵥱ʿ���:%lf\n",max_single_loss);
        printf("���س���ֵ:%lf\n",max_windrawl_number);
        printf("���س����俪ʼ:%s\n",max_windraw_start_date);
        printf("���س��������:%s\n",max_windraw_end_date);
        printf("�س���ȫϵ��:%lf\n",windrawl_safe_rate);
        printf("�ܽ��׳ɱ�:%lf\n",total_transcaction_fee);
        printf("ƽ�����׳ɱ�:%lf\n",ave_transcation_fee);
        printf("���׳ɱ�������:%lf\n",transcation_cover_rate);
        printf("ʵ��������:%lf\n",real_income_rate);
        printf("ƽ��ÿ��������:%lf\n",ave_time_income_rate);
        printf("������:%lf\n",variance);
        printf("���:%lf\n",kurtosis);
        printf("ƫ��:%lf\n",skewness);
        printf("����ŵ����:%lf\n",sortinoRatio);
        printf("Ŀ�꺯��������ʵ�ʷ���:%lf\n",targetFunctionRankScore);
        printf("Ŀ�꺯��Prom��ʵ�ʷ���:%lf\n",targetFunctionPromScore);

    }
};
void showResult(){
    printf("tradingNum: %d\n",tradingNum);
    double capital = 0.0;
    double long_capital = 0.0, short_capital = 0.0;
    double total_commission = 0.0, total_slippages = 0.0;
    double max_long_income = 0.0 ;
    for(int i = 0;i < tradingNum; i++){
        double turnover = (tradingResult[i].entryPrice + tradingResult[i].exitPrice) * perSize * fabs(tradingResult[i].volume);
        double commission = turnover * rate;
        double per_slippage = slippage * 2 * perSize * fabs(tradingResult[i].volume);
        double pnl = (tradingResult[i].exitPrice - tradingResult[i].entryPrice) * tradingResult[i].volume * perSize - commission - per_slippage;
        #ifdef debug
        printf("entryTime: %s , entry price:%lf ,exitTime%s : exit price:%lf\n",tradingResult[i].entryTime, tradingResult[i].entryPrice, tradingResult[i].exitTime, tradingResult[i].exitPrice);
        #endif // debug
        total_commission += commission;
        capital += pnl ;
        total_slippages += per_slippage;
        if(tradingResult[i].volume > 0){
            long_capital += pnl ;
            max_long_income = max(max_long_income , pnl );
        }
        if(tradingResult[i].volume < 0){
            short_capital += pnl ;
            max_long_income = max(max_long_income , pnl );
        }
    }
    printf("capital:%lf\nlong_capital:%lf\nshort_capital %lf\n",capital,long_capital,short_capital);
    printf("total_commision:%lf\ntotal_slippages:%lf\n",total_commission,total_slippages);
    printf("max_long_income:%lf\n",max_long_income);
}

void run(){

}

void init(){
    // init the params
    // loop canshu

    run();
    TradingResultNode a(tradingResult,tradingNum , account_money ,slippage, rate , vecTradeDay , vecTradeWeekDay);
    a.showResult();
    //showResult();
}

int main(){
    loadFile("rb888_30_minutes.csv" , "2009/03/27" , "2050/03/31");

    init();

    return 0;
}
